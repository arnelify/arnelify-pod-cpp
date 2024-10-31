#ifndef BOOST_UPLOADER_HPP
#define BOOST_UPLOADER_HPP

#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>

#include "boost/beast.hpp"
#include "json.h"
#include "magic.h"

class Uploader {
 private:
  const bool keepExtensions;
  const int maxFields;
  const int maxFieldsSizeTotalMb;
  const bool allowEmptyFiles;
  const int maxFiles;
  const int maxFilesSizeTotalMb;
  const int maxFileSizeMb;

  const std::optional<std::string> getBoundaries(const auto& request) {
    const auto& contentTypeKey = boost::beast::http::field::content_type;
    const std::string contentType = request[contentTypeKey].to_string();
    const int boundaryStart = contentType.find("boundary=");
    const bool hasBoundaryStart = boundaryStart != std::string::npos;
    if (!hasBoundaryStart) return std::nullopt;

    return "--" + contentType.substr(boundaryStart + 9);
  }

  const std::filesystem::path getRealName(const std::string& part,
                                          const std::size_t& filenameStart,
                                          const std::size_t& filenameEnd) {
    const int partStart = filenameStart + 10;
    const int partEnd = filenameEnd - filenameStart - 10;
    return part.substr(partStart, partEnd);
  }

  const std::string getExtension(const std::filesystem::path& real) {
    const std::string raw = real.extension().string();
    std::string ext;

    ext.reserve(raw.size());

    for (char c : raw) {
      bool isAllowedSymbol = std::isalnum(c) || c == '.';
      if (isAllowedSymbol) {
        ext += c;
      } else {
        ext += '_';
      }
    }

    return ext;
  }

  const std::string getFilename(const std::string& ext) {
    std::random_device random;
    using Milliseconds = std::chrono::milliseconds;
    const auto now = std::chrono::system_clock::now();
    const auto milliseconds =
        std::chrono::duration_cast<Milliseconds>(now.time_since_epoch())
            .count();

    std::mt19937 gen(random());
    std::uniform_int_distribution<> dis(0, 9);
    std::ostringstream oss;
    oss << milliseconds;

    for (int i = 0; i < 5; ++i) {
      oss << dis(gen);
    }

    if (this->keepExtensions) return oss.str() + ext;
    return oss.str();
  }

  static std::string getMimeType(const std::string& filePath) {
    const std::string defaultMime = "application/octet-stream";
    magic_t magic = magic_open(MAGIC_MIME_TYPE);
    if (magic == nullptr) return defaultMime;
    if (magic_load(magic, nullptr) != 0) {
      magic_close(magic);
      return defaultMime;
    }

    const char* mimeType = magic_file(magic, filePath.c_str());
    std::string mime = mimeType ? mimeType : defaultMime;
    magic_close(magic);
    return mime;
  }

  const bool writeFile(const std::filesystem::path& filepath,
                       const std::string& fileContent) {
    std::ofstream ofs(filepath.string(), std::ios::binary);
    ofs << fileContent;
    if (!ofs) return false;
    return true;
  }

  const std::string decodeUrl(const std::string& encoded) {
    std::string decoded;
    for (std::size_t i = 0; i < encoded.size(); ++i) {
      if (encoded[i] == '%') {
        if (i + 2 < encoded.size()) {
          const std::string hex = encoded.substr(i + 1, 2);
          char decodedChar = static_cast<char>(std::stoi(hex, nullptr, 16));
          decoded += decodedChar;
          i += 2;
        }
      } else if (encoded[i] == '+') {
        decoded += ' ';
      } else {
        decoded += encoded[i];
      }
    }
    return decoded;
  }

  const std::vector<std::string> splitKey(const std::string& key) {
    std::vector<std::string> parts;
    std::string current;
    bool inBracket = false;
    int bracketCount = 0;

    for (char ch : key) {
      if (ch == '[') {
        if (inBracket) return {key};
        if (!current.empty()) {
          parts.emplace_back(current);
          current.clear();
        }
        inBracket = true;
        bracketCount++;
      } else if (ch == ']') {
        if (!inBracket) return {key};
        inBracket = false;
        if (!current.empty()) {
          parts.emplace_back(current);
          current.clear();
        }
        bracketCount--;
      } else {
        current += ch;
      }
    }

    if (inBracket || bracketCount != 0) return {key};
    if (!current.empty()) {
      parts.emplace_back(current);
    }

    return parts;
  }

 public:
  Uploader(const bool a, const int b, const int c, const bool d, const int e,
           const int f, const int g)
      : keepExtensions(a),
        maxFields(b),
        maxFieldsSizeTotalMb(c),
        allowEmptyFiles(d),
        maxFiles(e),
        maxFilesSizeTotalMb(f),
        maxFileSizeMb(g) {}

  const Json::Value multipart(auto& request) {
    Json::Value body = Json::objectValue;
    Json::Value res = Json::objectValue;

    const std::string reqBody = request.body();
    const std::optional<std::string> boundaryOpt = this->getBoundaries(request);
    bool hasBoundaries = boundaryOpt.has_value();
    if (!hasBoundaries) {
      res["code"] = 409;
      res["error"] = "Invalid multipart/form-data.";
      return res;
    }

    int fieldCount = 0;
    const std::size_t maxFieldsSizeTotal =
        this->maxFieldsSizeTotalMb * 1024 * 1024;
    std::size_t fieldsSizeTotalCount = 0;

    int fileCount = 0;
    const std::size_t maxFilesSizeTotal =
        this->maxFilesSizeTotalMb * 1024 * 1024;
    std::size_t filesSizeTotalCount = 0;

    const std::size_t maxFileSize = this->maxFileSizeMb * 1024 * 1024;
    const std::string boundary = boundaryOpt.value();

    std::size_t pos = 0;
    std::string prevName = "";
    while ((pos = reqBody.find(boundary, pos)) != std::string::npos) {
      const std::size_t posEnd = reqBody.find(boundary, pos + boundary.size());
      const bool hasPosEnd = posEnd != std::string::npos;
      if (!hasPosEnd) break;

      /* Content Disposition start */
      const std::string part = reqBody.substr(pos, posEnd - pos);
      const std::size_t dispositionStart = part.find("Content-Disposition: ");
      const bool hasDispositionStart = dispositionStart != std::string::npos;
      if (!hasDispositionStart) {
        pos = posEnd;
        continue;
      }

      std::size_t nameStart = part.find("name=\"", dispositionStart);
      const bool hasNameStart = nameStart != std::string::npos;
      if (!hasNameStart) {
        res["code"] = 409;
        res["error"] = "Invalid multipart/form-data.";
        return res;
      }

      nameStart += 6;
      const std::size_t nameEnd = part.find("\"", nameStart);
      const bool hasNameEnd = nameEnd != std::string::npos;
      if (!hasNameEnd) {
        res["code"] = 409;
        res["error"] = "Invalid multipart/form-data.";
        return res;
      }

      const std::string name = part.substr(nameStart, nameEnd - nameStart);
      const bool isNameLen = name.empty() || name.length() > 255;
      if (isNameLen) {
        res["code"] = 409;
        res["error"] = "Invalid multipart/form-data.";
        return res;
      }

      const bool isDiff = name != prevName;
      if (isDiff) {
        prevName = name;
        fieldCount++;
      }

      std::vector<std::string> keys = this->splitKey(name);
      Json::Value* current = &body;

      for (size_t i = 0; i < keys.size(); ++i) {
        const std::string& key = keys[i];
        if (i == keys.size() - 1) {
          (*current)[key] = Json::Value(Json::arrayValue);
          current = &(*current)[key];
        } else {
          current = &(*current)[key];
        }
      }

      // UPLOADER_MAX_FIELDS
      const bool isMaxFields = fieldCount > this->maxFields;
      if (isMaxFields) {
        res["code"] = 409;
        res["error"] = "The maximum number of fields has been exceeded.";
        return res;
      }

      /* File name start */
      const std::size_t filenameStart =
          part.find("filename=\"", dispositionStart);
      const bool hasFilenameStart = filenameStart != std::string::npos;
      if (hasFilenameStart) {
        fileCount++;

        const std::size_t filenameEnd = part.find("\"", filenameStart + 10);
        const bool hasFilenameEnd = filenameEnd != std::string::npos;
        if (!hasFilenameEnd) {
          res["code"] = 409;
          res["error"] = "Invalid multipart/form-data.";
          return res;
        }

        // UPLOADER_KEEP_EXTENSIONS
        const std::filesystem::path real =
            this->getRealName(part, filenameStart, filenameEnd);
        const std::string ext = this->getExtension(real);

        const std::size_t contentStart = part.find("\r\n\r\n", filenameEnd) + 4;
        const std::string fileContent =
            part.substr(contentStart, part.size() - contentStart - 2);
        const std::size_t size = fileContent.size();

        // UPLOADER_ALLOW_EMPTY_FILES
        const bool isFileEmpty =
            fileContent.find_first_not_of(" \n\r\t") == std::string::npos;
        if (isFileEmpty) {
          if (!this->allowEmptyFiles) {
            res["code"] = 409;
            res["error"] = "Empty files are not allowed.";
            return res;
          }
        }

        // UPLOADER_MAX_FILES
        const bool isMaxFiles = fileCount > this->maxFiles;
        if (isMaxFiles) {
          res["code"] = 409;
          res["error"] = "The maximum number of files has been exceeded.";
          return res;
        }

        // UPLOADER_MAX_FILES_SIZE_TOTAL_MB
        filesSizeTotalCount += size;
        const bool isMaxFilesSizeTotal =
            filesSizeTotalCount > maxFilesSizeTotal;
        if (isMaxFilesSizeTotal) {
          res["code"] = 409;
          res["error"] = "The maximum size of files has been exceeded.";
          return res;
        }

        // UPLOADER_MAX_FILE_SIZE_MB
        const bool isMaxFileSize = size > maxFileSize;
        if (isMaxFileSize) {
          res["code"] = 409;
          res["error"] = "The maximum file size has been exceeded.";
          return res;
        }

        const std::filesystem::path uploadDir = "./src/storage/uploads";
        const bool hasUploadDir = std::filesystem::exists(uploadDir);
        if (!hasUploadDir) {
          std::filesystem::create_directory(uploadDir);
        }

        // UPLOADER_KEEP_EXTENSIONS
        const std::string filename = this->getFilename(ext);
        const std::filesystem::path filepath = uploadDir / filename;
        const bool isWriteFile = this->writeFile(filepath, fileContent);
        if (!isWriteFile) {
          res["code"] = 409;
          res["error"] = "Unable to save the file.";
          return res;
        }

        Json::Value file = Json::objectValue;
        std::string mime = this->getMimeType(filepath.string());
        file["ext"] = ext;
        file["mime"] = mime;
        file["name"] = filename;
        file["real"] = real.string();
        file["path"] = filepath.string();
        file["size"] = size;
        current->append(file);

      } else {
        const std::size_t valueStart = part.find("\r\n\r\n", nameEnd) + 4;
        const bool hasValueStart = valueStart != std::string::npos;
        if (!hasValueStart) {
          res["code"] = 409;
          res["error"] = "Invalid multipart/form-data.";
          return res;
        }

        const std::string value =
            part.substr(valueStart, part.size() - valueStart - 2);

        // UPLOADER_MAX_FIELDS_SIZE_TOTAL_MB
        fieldsSizeTotalCount += value.size();
        const bool isMaxFieldsSizeTotal =
            fieldsSizeTotalCount > maxFieldsSizeTotal;
        if (isMaxFieldsSizeTotal) {
          res["code"] = 409;
          res["error"] = "The maximum size of fields has been exceeded.";
          return res;
        }

        current->append(value);
      }

      pos = posEnd;
    }

    res["code"] = 200;
    res["success"] = body;
    return res;
  }

  const Json::Value urlEncoded(const auto& request) {
    Json::Value body = Json::objectValue;
    Json::Value res = Json::objectValue;

    const std::string reqBody = request.body();
    const std::size_t reqBodyLength = reqBody.size();

    int fieldCount = 0;
    const std::size_t maxFieldsSizeTotal =
        this->maxFieldsSizeTotalMb * 1024 * 1024;
    std::size_t fieldsSizeTotalCount = 0;

    std::size_t pos = 0;
    std::string prevName = "";
    while (pos < reqBodyLength) {
      std::size_t nextPos = reqBody.find('&', pos);
      const bool hasNext = nextPos == std::string::npos;
      if (hasNext) nextPos = reqBodyLength;

      std::string pair = reqBody.substr(pos, nextPos - pos);
      std::size_t equalPos = pair.find('=');
      const bool hasEqual = equalPos != std::string::npos;
      if (hasEqual) {
        const std::string rawKey = pair.substr(0, equalPos);
        const std::string name = this->decodeUrl(rawKey);
        const std::string rawValue = pair.substr(equalPos + 1);
        const std::string value = this->decodeUrl(rawValue);

        const bool isDiff = name != prevName;
        if (isDiff) {
          prevName = name;
          fieldCount++;
        }

        // UPLOADER_MAX_FIELDS
        const bool isMaxFields = fieldCount > this->maxFields;
        if (isMaxFields) {
          res["code"] = 409;
          res["error"] = "The maximum number of fields has been exceeded.";
          return res;
        }

        // UPLOADER_MAX_FIELDS_SIZE_TOTAL_MB
        fieldsSizeTotalCount += value.size();
        const bool isMaxFieldsSizeTotal =
            fieldsSizeTotalCount > maxFieldsSizeTotal;
        if (isMaxFieldsSizeTotal) {
          res["code"] = 409;
          res["error"] = "The maximum size of fields has been exceeded.";
          return res;
        }

        std::vector<std::string> keys = this->splitKey(name);
        Json::Value* current = &body;
        for (const auto& k : keys) {
          current = &(*current)[k];
        }

        current->append(value);
      }

      pos = nextPos + 1;
    }

    res["code"] = 200;
    res["success"] = body;
    return res;
  }

  const Json::Value parse(const auto& request) {
    const auto& contentTypeKey = boost::beast::http::field::content_type;
    const auto& contentType = request[contentTypeKey];

    const std::size_t contentTypeStart =
        contentType.find("multipart/form-data");
    const bool isMultipartFormData = contentTypeStart != std::string::npos;
    if (isMultipartFormData) return this->multipart(request);
    return this->urlEncoded(request);
  }
};

#endif