#ifndef ARNELIFY_RECEIVER_HPP
#define ARNELIFY_RECEIVER_HPP

#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>

#include "json.h"

#include "contracts/opts.hpp"

using ArnelifyServerReq = Json::Value;

class ArnelifyReceiver final {
 private:
  int SIGNAL_ACCEPTED;
  int SIGNAL_ERROR;
  int SIGNAL_FINISH;

  bool hasBody;
  bool hasHeaders;
  bool hasMethod;
  bool hasPath;
  bool hasVersion;

  bool startSize;
  const ArnelifyReceiverOpts opts;
  std::string buffer;
  ArnelifyServerReq req;

  std::string acceptEncoding;
  std::string contentType;
  std::string boundary;
  std::vector<std::string> prefixes;
  std::string status;

  std::size_t length;
  std::string mime;
  std::string name;
  int size;

  std::vector<std::string> fields;
  std::size_t fieldsSizeTotal;
  std::string body;

  int filesCounter;
  std::size_t filesSizeTotal;
  std::string fileExt;
  std::filesystem::path filePath;
  std::string fileReal;
  std::size_t fileSize;
  bool isWrite;

  void addKey(const std::string& name) {
    const std::size_t patternStart = name.find("[");
    const bool isPattern = patternStart != std::string::npos;
    if (isPattern) {
      const std::string key = name.substr(0, patternStart);
      const bool hasKey = std::find(this->fields.begin(), this->fields.end(),
                                    key) != fields.end();
      if (!hasKey) this->fields.emplace_back(key);
      return;
    }

    const bool hasKey = std::find(this->fields.begin(), this->fields.end(),
                                  name) != fields.end();
    if (!hasKey) this->fields.emplace_back(name);
  }

  const std::string decode(const std::string& encoded) {
    std::string decoded;
    int hex;

    for (size_t i = 0; i < encoded.length(); ++i) {
      const bool isHex = encoded[i] == '%';
      if (isHex) {
        if (i + 2 < encoded.length() &&
            sscanf(encoded.substr(i + 1, 2).c_str(), "%x", &hex) == 1) {
          decoded += static_cast<char>(hex);
          i += 2;
        }

        continue;
      }

      const bool isSpace = encoded[i] == '+';
      if (isSpace) {
        decoded += ' ';
        continue;
      }

      decoded += encoded[i];
    }

    return decoded;
  }

  const std::vector<std::string> getKeys(const std::string& name) {
    const std::size_t patternStart = name.find("[]");
    const bool hasPattern = patternStart != std::string::npos;
    if (!hasPattern) return {name};

    std::vector<std::string> keys;
    std::string buffer = name.substr(0, patternStart);

    std::size_t keyStart = buffer.find("[");
    const bool hasKeyStart = keyStart != std::string::npos;
    if (!hasKeyStart) return {buffer};

    keys.emplace_back(buffer.substr(0, keyStart));
    buffer = buffer.substr(keyStart);
    keyStart = buffer.find("[");

    while (keyStart != std::string::npos) {
      const std::size_t keyEnd = buffer.find("]", keyStart + 1);
      const bool hasKeyEnd = keyEnd != std::string::npos;
      if (!hasKeyEnd) return {name};

      const std::string key =
          buffer.substr(keyStart + 1, keyEnd - keyStart - 1);
      const std::size_t innerStart = key.find("[");
      const bool hasInner = innerStart != std::string::npos;
      if (hasInner) return {name};

      keys.emplace_back(key);
      keyStart = buffer.find("[", keyEnd + 1);
    }

    return keys;
  }

  void setBody() {
    Json::Value* current = &req["body"];
    const std::vector<std::string> keys = getKeys(this->name);
    for (const auto& key : keys) {
      if (current->isArray()) return;
      current = &(*current)[key];
    }

    const bool isArray = (*current).isArray();
    if (!isArray) (*current) = Json::arrayValue;
    (*current).append(this->body);
  }

  int setBoundary(const std::string& value) {
    const int boundaryStart = value.find("boundary=");
    const bool hasBoundary = boundaryStart != std::string::npos;
    if (!hasBoundary) return this->SIGNAL_ACCEPTED;

    this->boundary = "--" + value.substr(boundaryStart + 9);

    for (std::size_t i = 1; this->boundary.length() >= i; ++i) {
      const std::string prefix = this->boundary.substr(0, i);
      const bool isPrefix = this->boundary != prefix;
      if (isPrefix) this->prefixes.emplace_back(prefix);
    }

    return this->SIGNAL_FINISH;
  }

  int setCookie(const std::string& value) {
    bool isFirst = true;
    std::stringstream ss(value);
    std::string param;
    while (std::getline(ss, param, ';')) {
      const std::size_t equalStart = param.find('=');
      const bool hasEqual = equalStart != std::string::npos;
      if (hasEqual) {
        if (isFirst) {
          const std::string name = param.substr(0, equalStart);
          const std::string data = param.substr(equalStart + 1);
          this->req["_state"]["cookie"][name] = data;
          isFirst = false;
          continue;
        }

        const std::string name = param.substr(1, equalStart - 1);
        const std::string data = param.substr(equalStart + 1);
        this->req["_state"]["cookie"][name] = data;
      }
    }

    return this->SIGNAL_FINISH;
  }

  void setFile() {
    Json::Value file = Json::objectValue;
    file["ext"] = this->fileExt;
    file["mime"] = this->mime;
    file["name"] = std::string(this->filePath.stem());
    file["path"] = std::string(this->filePath);
    file["real"] = this->fileReal;
    file["size"] = Json::UInt64(this->fileSize);
    this->isWrite = false;

    Json::Value* current = &this->req["files"];
    std::vector<std::string> keys = this->getKeys(this->name);
    for (const auto& key : keys) {
      if (current->isArray()) return;
      current = &(*current)[key];
    }

    const bool isArray = (*current).isArray();
    if (!isArray) (*current) = Json::arrayValue;
    (*current).append(file);
  }

  int setFilename(const std::string& value) {
    std::random_device random;
    const auto now = std::chrono::system_clock::now();
    const auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch())
            .count();

    std::mt19937 gen(random());
    std::uniform_int_distribution<> dis(0, 9);
    std::ostringstream oss;
    oss << milliseconds;

    for (int i = 0; i < 5; ++i) {
      oss << dis(gen);
    }

    this->fileExt = "";
    this->fileSize = 0;
    this->fileReal = value;
    const std::filesystem::path path(this->fileReal);
    const std::string raw = path.extension().string();
    this->fileExt.reserve(raw.size());
    for (char c : raw) {
      bool isAllowedSymbol = std::isalnum(c) || c == '.';
      if (isAllowedSymbol) {
        this->fileExt += c;
      } else {
        this->fileExt += '_';
      }
    }

    if (this->opts.RECEIVER_KEEP_EXTENSIONS) {
      const std::string file = oss.str() + this->fileExt;
      this->filePath = this->opts.RECEIVER_UPLOAD_DIR / file;
      return this->SIGNAL_FINISH;
    }

    this->filePath = this->opts.RECEIVER_UPLOAD_DIR / oss.str();
    return this->SIGNAL_FINISH;
  }

  int setHeader(const std::string& key, const std::string& value) {
    this->req["_state"]["headers"][key] = value;

    const bool isAcceptEncoding = key == "Accept-Encoding";
    if (isAcceptEncoding) {
      this->acceptEncoding = value;
      return this->SIGNAL_FINISH;
    }

    const bool isContentLength = key == "Content-Length";
    if (isContentLength) {
      for (char c : value) {
        if (!isdigit(c)) {
          this->status = "Content-Length must be a number.";
          return this->SIGNAL_ERROR;
        }
      }

      this->length = std::stoull(value);
      return this->SIGNAL_FINISH;
    }

    const bool isContentType = key == "Content-Type";
    if (isContentType) {
      if (this->hasBody) {
        this->contentType = value;
        return this->SIGNAL_FINISH;
      }

      const std::size_t semicolonStart = value.find(";");
      const bool hasSemicolon = semicolonStart != std::string::npos;
      if (!hasSemicolon) {
        this->contentType = value;
        return this->SIGNAL_FINISH;
      }

      this->contentType = value.substr(0, semicolonStart);
      const int SIGNAL_BOUNDARY = this->setBoundary(value);
      if (SIGNAL_BOUNDARY != this->SIGNAL_FINISH) return SIGNAL_BOUNDARY;
    }

    const bool isCookie = key == "Cookie";
    if (isCookie) {
      const int SIGNAL_COOKIE = this->setCookie(value);
      if (SIGNAL_COOKIE != this->SIGNAL_FINISH) return SIGNAL_COOKIE;
    }

    return this->SIGNAL_FINISH;
  }

  int setQuery(const std::string& param, const std::string query) {
    std::stringstream ss(query);
    std::string pair;

    while (std::getline(ss, pair, '&')) {
      const std::size_t equalStart = pair.find("=");
      const bool hasEqual = equalStart != std::string::npos;

      const std::string name = pair.substr(0, equalStart);
      const std::string value = pair.substr(equalStart + 1);

      bool isValidKey = true;
      Json::Value* current = &req[param];
      const std::vector<std::string> keys = this->getKeys(name);
      for (const auto& key : keys) {
        if (current->isArray()) {
          isValidKey = false;
          break;
        }

        current = &(*current)[key];
      }

      if (!isValidKey) continue;
      const bool isArray = (*current).isArray();
      if (!isArray) (*current) = Json::arrayValue;
      (*current).append(value);
    }

    return this->SIGNAL_FINISH;
  }

  int onMethod() {
    const std::size_t methodEnd = this->buffer.find(' ');
    const bool hasMethod = methodEnd != std::string::npos;
    if (!hasMethod) {
      const int isMaxMethodSize = this->buffer.length() > 8;
      if (isMaxMethodSize) {
        this->status = "The maximum size of the method has been exceeded.";
        return this->SIGNAL_ERROR;
      }

      return this->SIGNAL_ACCEPTED;
    }

    const int isMaxMethodSize = methodEnd - 1 > 8;
    if (isMaxMethodSize) {
      this->status = "The maximum size of the method has been exceeded.";
      return this->SIGNAL_ERROR;
    }

    const std::string method = this->buffer.substr(0, methodEnd);
    const bool isSupport =
        method == "GET" || method == "POST" || method == "PUT" ||
        method == "DELETE" || method == "HEAD" || method == "OPTIONS" ||
        method == "PATCH" || method == "CONNECT" || method == "TRACE";
    if (!isSupport) {
      this->status = "Unknown request method.";
      return this->SIGNAL_ERROR;
    }

    this->hasBody = !(method == "PATCH" || method == "POST" ||
                      method == "PUT" || method == "DELETE");
    this->req["_state"]["method"] = method;
    this->buffer = this->buffer.substr(methodEnd + 1);
    this->hasMethod = true;

    return this->SIGNAL_FINISH;
  }

  int onPath() {
    const std::size_t urlEnd = this->buffer.find(' ');
    const bool hasUrl = urlEnd != std::string::npos;
    if (!hasUrl) {
      const int isMaxUrlSize = this->buffer.length() > 2048;
      if (isMaxUrlSize) {
        this->status = "The maximum size of the URL has been exceeded.";
        return this->SIGNAL_ERROR;
      }

      return this->SIGNAL_ACCEPTED;
    }

    const int isMaxUrlSize = urlEnd - 1 > 2048;
    if (isMaxUrlSize) {
      this->status = "The maximum size of the URL has been exceeded.";
      return this->SIGNAL_ERROR;
    }

    const std::string url = this->buffer.substr(0, urlEnd);
    const std::size_t queryStart = url.find('?');
    const bool hasQuery = queryStart != std::string::npos;
    if (hasQuery) {
      this->req["_state"]["path"] = url.substr(0, queryStart);
      const std::string encoded = url.substr(queryStart + 1);
      const std::string decoded = this->decode(encoded);
      this->setQuery("query", decoded);
    } else {
      this->req["_state"]["path"] = url;
    }

    this->buffer = this->buffer.substr(urlEnd + 1);
    this->hasPath = true;

    return this->SIGNAL_FINISH;
  }

  int onVersion() {
    const std::size_t versionEnd = this->buffer.find("\r\n");
    const bool hasVersion = versionEnd != std::string::npos;
    if (!hasVersion) {
      const int isMaxVersionSize = this->buffer.length() > 10;
      if (isMaxVersionSize) {
        this->status = "The maximum size of the version has been exceeded.";
        return this->SIGNAL_ERROR;
      }

      return this->SIGNAL_ACCEPTED;
    }

    const int isMaxVersionSize = versionEnd - 1 > 10;
    if (isMaxVersionSize) {
      this->status = "The maximum size of the version has been exceeded.";
      return this->SIGNAL_ERROR;
    }

    this->req["_state"]["version"] = this->buffer.substr(0, versionEnd);
    this->buffer = this->buffer.substr(versionEnd + 2);
    this->hasVersion = true;
    return this->SIGNAL_FINISH;
  }

  int onHeaders() {
    const std::size_t headersEnd = this->buffer.find("\r\n\r\n");
    const bool hasHeaders = headersEnd != std::string::npos;
    if (!hasHeaders) {
      const int isMaxHeadersSize = this->buffer.length() > 8192;
      if (isMaxHeadersSize) {
        this->status = "The maximum size of headers has been exceeded.";
        return this->SIGNAL_ERROR;
      }

      return this->SIGNAL_ACCEPTED;
    }

    const int isMaxHeadersSize = headersEnd - 1 > 8192;
    if (isMaxHeadersSize) {
      this->status = "The maximum size of headers has been exceeded.";
      return this->SIGNAL_ERROR;
    }

    std::string headers = this->buffer.substr(0, headersEnd + 2);
    std::size_t headerEnd = headers.find("\r\n");
    while (headerEnd != std::string::npos) {
      const std::string header = headers.substr(0, headerEnd);
      const std::size_t colonStart = header.find(": ");

      const bool hasColon = colonStart != std::string::npos;
      if (hasColon) {
        const std::string key = header.substr(0, colonStart);
        const std::string value = header.substr(colonStart + 2);
        const int SIGNAL_HEADER = this->setHeader(key, value);
        if (SIGNAL_HEADER != this->SIGNAL_FINISH) return SIGNAL_HEADER;
      }

      headers = headers.substr(headerEnd + 2);
      headerEnd = headers.find("\r\n");
    }

    this->buffer = this->buffer.substr(headersEnd + 4);
    this->size = this->buffer.length();
    this->hasHeaders = true;

    return this->SIGNAL_FINISH;
  }

  int onMeta(std::string& meta) {
    std::size_t metaEnd = meta.find("\r\n");
    while (metaEnd != std::string::npos) {
      const std::string header = meta.substr(0, metaEnd);
      const bool hasContentDisposition =
          header.starts_with("Content-Disposition");
      if (hasContentDisposition) {
        const std::size_t nameStart = header.find("name=\"");
        const bool hasNameStart = nameStart != std::string::npos;
        if (!hasNameStart) {
          this->status = "Invalid name detected in multipart/form-data.";
          return this->SIGNAL_ERROR;
        }

        const std::size_t nameEnd = header.find("\"", nameStart + 6);
        const bool hasNameEnd = nameEnd != std::string::npos;
        if (!hasNameEnd) {
          this->status = "Invalid name detected in multipart/form-data.";
          return this->SIGNAL_ERROR;
        }

        const std::size_t nameSize = nameEnd - (nameStart + 6);
        const bool isMaxNameSize = nameSize > 2048;
        if (isMaxNameSize) {
          this->status = "Invalid name detected in multipart/form-data.";
          return this->SIGNAL_ERROR;
        }

        const std::string name = header.substr(nameStart + 6, nameSize);

        this->addKey(name);
        const bool isMaxFields =
            this->fields.size() > this->opts.RECEIVER_MAX_FIELDS;
        if (isMaxFields) {
          this->status = "The maximum number of fields has been exceeded.";
          return this->SIGNAL_ERROR;
        }

        this->name = name;
        this->body.clear();

        const std::size_t filenameStart = header.find("filename=\"");
        const bool hasFilenameStart = filenameStart != std::string::npos;
        if (hasFilenameStart) {
          const std::size_t filenameEnd = header.find("\"", filenameStart + 10);
          const bool hasFilenameEnd = filenameEnd != std::string::npos;
          if (!hasFilenameEnd) {
            this->status = "Invalid filename detected in multipart/form-data.";
            return this->SIGNAL_ERROR;
          }

          const bool isMaxFilename = filenameEnd - filenameStart + 10 > 255;
          if (isMaxFilename) {
            this->status = "Invalid filename detected in multipart/form-data.";
            return this->SIGNAL_ERROR;
          }

          const bool hasFile = filenameStart + 10 != filenameEnd;
          if (hasFile) {
            this->filesCounter += 1;

            const bool isMaxFiles =
                this->filesCounter > this->opts.RECEIVER_MAX_FILES;
            if (isMaxFiles) {
              this->status = "The maximum number of files has been exceeded.";
              return this->SIGNAL_ERROR;
            }

            const std::string filename = header.substr(
                filenameStart + 10, filenameEnd - (filenameStart + 10));
            const int SIGNAL_FILE = this->setFilename(filename);
            if (SIGNAL_FILE != this->SIGNAL_FINISH) return SIGNAL_FILE;

            this->isWrite = true;
          }
        }
      }

      const bool hasContentType = header.starts_with("Content-Type");
      if (hasContentType) {
        const std::size_t mimeStart = header.find(": ");
        const bool hasMime = mimeStart != std::string::npos;
        if (!hasMime) {
          this->status = "Invalid mime detected in multipart/form-data.";
          return this->SIGNAL_ERROR;
        }

        const std::string mime = header.substr(mimeStart + 2);
        this->mime = mime;
      }

      meta = meta.substr(metaEnd + 2);
      metaEnd = meta.find("\r\n");
    }

    return this->SIGNAL_FINISH;
  }

  int onJson() {
    const bool bodyEnd = this->size == this->length;
    if (bodyEnd) {
      Json::Value body;
      Json::CharReaderBuilder reader;
      std::string errors;

      std::istringstream iss(this->buffer);
      if (!Json::parseFromStream(reader, iss, &this->req["body"], &errors)) {
        this->status = "The body contains invalid JSON.";
        return this->SIGNAL_ERROR;
      }

      this->hasBody = true;
      return this->SIGNAL_FINISH;
    }

    return this->SIGNAL_ACCEPTED;
  }

  int onMultipart() {
    for (const auto& prefix : this->prefixes) {
      const bool hasPrefix = this->buffer.ends_with(prefix);
      if (hasPrefix) return this->SIGNAL_ACCEPTED;
    }

    std::size_t boundaryStart = this->buffer.find(this->boundary);
    while (boundaryStart != std::string::npos) {
      const std::string before = this->buffer.substr(0, boundaryStart);
      const bool hasBefore = before.length() >= 2;
      if (hasBefore) {
        const std::size_t beforeLen = before.length() - 2;
        if (this->isWrite) {
          const bool isAllowEmptyFiles =
              !this->opts.RECEIVER_ALLOW_EMPTY_FILES && !beforeLen;
          if (isAllowEmptyFiles) {
            this->status = "Empty files are not allowed.";
            return this->SIGNAL_ERROR;
          }

          this->fileSize += beforeLen;
          this->filesSizeTotal += beforeLen;

          const bool isMaxFileSize =
              this->fileSize >
              this->opts.RECEIVER_MAX_FILE_SIZE_MB * 1024 * 1024;
          if (isMaxFileSize) {
            this->status = "The maximum size of the file has been exceeded.";
            return this->SIGNAL_ERROR;
          }

          const bool isMaxFilesSizeTotal =
              this->filesSizeTotal >
              this->opts.RECEIVER_MAX_FILES_SIZE_TOTAL_MB * 1024 * 1024;
          if (isMaxFilesSizeTotal) {
            this->status = "The maximum size of files has been exceeded.";
            return this->SIGNAL_ERROR;
          }

          const int SIGNAL_ON_WRITE =
              this->onWrite(before.substr(0, beforeLen));
          if (SIGNAL_ON_WRITE != this->SIGNAL_FINISH) return SIGNAL_ON_WRITE;
          this->setFile();

        } else {
          this->fieldsSizeTotal += beforeLen;

          const bool isMaxFieldsSizeTotal =
              this->fieldsSizeTotal >
              this->opts.RECEIVER_MAX_FIELDS_SIZE_TOTAL_MB * 1024 * 1024;
          if (isMaxFieldsSizeTotal) {
            this->status = "The maximum size of fields has been exceeded.";
            return this->SIGNAL_ERROR;
          }

          this->body.append(before.substr(0, beforeLen));
          this->setBody();
        }

        this->buffer = this->buffer.substr(boundaryStart);
        boundaryStart = this->buffer.find(this->boundary);
      }

      const std::string bodyEnd = this->boundary + "--\r\n";
      const bool hasBodyEnd = this->buffer.starts_with(bodyEnd);
      if (hasBodyEnd) {
        this->hasBody = true;
        return this->SIGNAL_FINISH;
      }

      const std::size_t boundaryEnd = this->buffer.find("\r\n", boundaryStart);
      const bool hasBoundaryEnd = boundaryEnd != std::string::npos;
      if (!hasBoundaryEnd) return this->SIGNAL_ACCEPTED;

      const std::size_t metaEnd =
          this->buffer.find("\r\n\r\n", boundaryEnd + 2);
      const bool hasMetas = metaEnd != std::string::npos;
      if (!hasMetas) return this->SIGNAL_ACCEPTED;

      std::string meta =
          this->buffer.substr(boundaryEnd + 2, metaEnd - boundaryEnd);
      const int SIGNAL_ON_META = this->onMeta(meta);
      if (SIGNAL_ON_META != this->SIGNAL_FINISH) return SIGNAL_ON_META;

      this->buffer = this->buffer.substr(metaEnd + 4);
      boundaryStart = this->buffer.find(this->boundary);
    }

    if (this->isWrite) {
      if (this->buffer.empty()) return this->SIGNAL_ACCEPTED;
      this->filesSizeTotal += this->buffer.length();
      this->fileSize += this->buffer.length();

      const bool isMaxFileSize =
          this->fileSize > this->opts.RECEIVER_MAX_FILE_SIZE_MB * 1024 * 1024;
      if (isMaxFileSize) {
        this->status = "The maximum size of the file has been exceeded.";
        return this->SIGNAL_ERROR;
      }

      const bool isMaxFilesSizeTotal =
          this->filesSizeTotal >
          this->opts.RECEIVER_MAX_FILES_SIZE_TOTAL_MB * 1024 * 1024;
      if (isMaxFilesSizeTotal) {
        this->status = "The maximum size of files has been exceeded.";
        return this->SIGNAL_ERROR;
      }

      const int SIGNAL_ON_WRITE = this->onWrite(this->buffer);
      if (SIGNAL_ON_WRITE != SIGNAL_FINISH) return SIGNAL_ON_WRITE;
      this->buffer.clear();

      return this->SIGNAL_ACCEPTED;
    }

    this->fieldsSizeTotal += this->buffer.length();

    const bool isMaxFieldsSizeTotal =
        this->fieldsSizeTotal >
        this->opts.RECEIVER_MAX_FIELDS_SIZE_TOTAL_MB * 1024 * 1024;
    if (isMaxFieldsSizeTotal) {
      this->status = "The maximum size of fields has been exceeded.";
      return this->SIGNAL_ERROR;
    }

    this->body.append(this->buffer);
    this->buffer.clear();

    return this->SIGNAL_ACCEPTED;
  }

  int onUrlEncoded() {
    const bool bodyEnd = this->size == this->length;
    if (bodyEnd) {
      const std::string decoded = this->decode(this->buffer);
      this->setQuery("body", decoded);
      this->hasBody = true;

      return this->SIGNAL_FINISH;
    }

    return this->SIGNAL_ACCEPTED;
  }

  int onWrite(const std::string& block) {
    std::ofstream file(this->filePath, std::ios::app);
    const bool isOpen = file.is_open();
    if (!isOpen) {
      this->status = "Can't save file.";
      return this->SIGNAL_ERROR;
    }

    file << block;
    file.close();

    return this->SIGNAL_FINISH;
  }

 public:
  ArnelifyReceiver(ArnelifyReceiverOpts& o)
      : SIGNAL_ACCEPTED(0),
        SIGNAL_ERROR(1),
        SIGNAL_FINISH(2),

        opts(o),
        length(0),

        hasBody(false),
        hasHeaders(false),
        hasMethod(false),
        hasPath(false),
        hasVersion(false),

        startSize(false),
        size(0),

        fieldsSizeTotal(0),
        filesCounter(0),
        filesSizeTotal(0),
        isWrite(false) {
    this->status = "Invalid request.";
    this->req["body"] = Json::objectValue;
    this->req["files"] = Json::objectValue;
    this->req["query"] = Json::objectValue;

    Json::Value _state;
    _state["client"] = this->opts.RECEIVER_CLIENT;
    _state["cookie"] = Json::objectValue;
    _state["headers"] = Json::objectValue;
    this->req["_state"] = _state;
  }

  int onBlock(const char* block, const std::size_t& bytesRead) {
    this->buffer.append(block, bytesRead);

    if (!this->hasMethod) {
      const int SIGNAL_ON_METHOD = this->onMethod();
      if (SIGNAL_ON_METHOD != this->SIGNAL_FINISH) return SIGNAL_ON_METHOD;
    }

    if (!this->hasPath) {
      const int SIGNAL_ON_PATH = this->onPath();
      if (SIGNAL_ON_PATH != this->SIGNAL_FINISH) return SIGNAL_ON_PATH;
    }

    if (!this->hasVersion) {
      const int SIGNAL_ON_VERSION = this->onVersion();
      if (SIGNAL_ON_VERSION != this->SIGNAL_FINISH) return SIGNAL_ON_VERSION;
    }

    if (!this->hasHeaders) {
      const int SIGNAL_ON_HEADERS = this->onHeaders();
      if (SIGNAL_ON_HEADERS != this->SIGNAL_FINISH) return SIGNAL_ON_HEADERS;
    }

    if (!this->hasBody) {
      if (this->startSize) this->size += bytesRead;
      if (!this->startSize) this->startSize = true;
      const bool isError = this->size > this->length;
      if (isError) {
        this->status = "The body exceeds the Content-Length.";
        return this->SIGNAL_ERROR;
      }

      const bool isJson = this->contentType == "application/json";
      if (isJson) return this->onJson();

      const bool isUrlEncoded =
          this->contentType == "application/x-www-form-urlencoded";
      if (isUrlEncoded) return this->onUrlEncoded();

      const bool isMultipart =
          this->contentType == "multipart/form-data" && !this->boundary.empty();
      if (isMultipart) return this->onMultipart();
      return this->onUrlEncoded();
    }

    return this->SIGNAL_FINISH;
  }

  const std::string getEncoding() { return this->acceptEncoding; }

  const std::string getStatus() { return this->status; }

  const Json::Value finish() {
    this->hasBody = false;
    this->hasHeaders = false;
    this->hasMethod = false;
    this->hasPath = false;
    this->hasVersion = false;
    this->startSize = false;

    this->buffer.clear();

    this->acceptEncoding.clear();
    this->contentType.clear();
    this->boundary.clear();
    this->prefixes.clear();
    this->status.clear();

    this->length = 0;
    this->mime.clear();
    this->name.clear();
    this->size = 0;

    this->fields.clear();
    this->fieldsSizeTotal = 0;
    this->body.clear();

    this->filesCounter = 0;
    this->filesSizeTotal = 0;
    this->fileExt.clear();
    this->filePath.clear();
    this->fileReal.clear();
    this->fileSize = 0;

    this->isWrite = false;
    return this->req;
  }
};

#endif