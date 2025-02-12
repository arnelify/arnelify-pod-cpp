#ifndef ARNELIFY_TRANSMITTER_HPP
#define ARNELIFY_TRANSMITTER_HPP

#include <arpa/inet.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <unistd.h>
#include <zlib.h>

#include "contracts/callback.hpp"
#include "contracts/opts.hpp"

class ArnelifyTransmitter final {
 private:
  std::size_t blockSize;
  std::string body;
  ArnelifyTransmitterCallback callback = [](const std::string &message,
                                            const bool &isError) {
    if (isError) {
      std::cout << "[Arnelify Server]: Error: " << message << std::endl;
      return;
    }

    std::cout << "[Arnelify Server]: " << message << std::endl;
  };

  int code;
  std::filesystem::path filePath;
  bool isGzip;
  bool isStatic;
  std::map<std::string, std::string> headers;
  const ArnelifyTransmitterOpts opts;
  const int socket;

  const std::string getMime(const std::string &extension) {
    const std::string &charset = this->opts.TRANSMITTER_CHARSET;
    const std::map<const std::string, const std::string> mime = {
        {".avi", "video/x-msvideo"},
        {".css", "text/css; charset=" + charset},
        {".csv", "text/csv; charset=" + charset},
        {".eot", "font/eot"},
        {".gif", "image/gif"},
        {".htm", "text/html; charset=" + charset},
        {".html", "text/html; charset=" + charset},
        {".ico", "image/x-icon"},
        {".jpeg", "image/jpeg"},
        {".jpg", "image/jpeg"},
        {".js", "application/javascript; charset=" + charset},
        {".json", "application/json; charset=" + charset},
        {".mkv", "video/x-matroska"},
        {".mov", "video/quicktime"},
        {".mp3", "audio/mpeg"},
        {".mp4", "video/mp4"},
        {".otf", "font/otf"},
        {".png", "image/png"},
        {".svg", "image/svg+xml; charset=" + charset},
        {".ttf", "font/ttf"},
        {".txt", "text/plain; charset=" + charset},
        {".wasm", "application/wasm"},
        {".wav", "audio/wav"},
        {".weba", "audio/webm"},
        {".webp", "image/webp"},
        {".woff", "font/woff"},
        {".woff2", "font/woff2"},
        {".xml", "application/xml; charset=" + charset}};

    auto it = mime.find(extension);
    const bool hasMime = it != mime.end();
    if (hasMime) {
      return it->second;
    }

    return "application/octet-stream";
  }

  const void gzip(const char *data, const uInt &bytesRead,
                  unsigned char *&compressed, std::size_t &bytesCompressed) {
    z_stream zlib;
    zlib.zalloc = Z_NULL;
    zlib.zfree = Z_NULL;
    zlib.opaque = Z_NULL;
    int ret = deflateInit2(&zlib, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8,
                           Z_DEFAULT_STRATEGY);
    if (ret != Z_OK) {
      this->callback("deflateInit2 failed with error: ", true);
      std::cout << "Zlib error: " << ret << std::endl;
      deflateEnd(&zlib);
      return;
    }

    zlib.next_in = (unsigned char *)data;
    zlib.avail_in = bytesRead;

    zlib.avail_out = this->blockSize;
    zlib.next_out = compressed;
    ret = deflate(&zlib, Z_FINISH);
    if (ret == Z_STREAM_ERROR) {
      this->callback("deflateInit2 failed with error: ", true);
      std::cout << "Zlib error: " << ret << std::endl;
      deflateEnd(&zlib);
      return;
    }

    bytesCompressed += this->blockSize - zlib.avail_out;
    deflateEnd(&zlib);
  }

  void resetHeaders(const bool &init = false) {
    if (!init) this->headers.clear();

    this->headers["Connection"] = "close";
    this->headers["Content-Length"] = "0";
    this->headers["Content-Type"] = this->getMime(".json");
    this->headers["Server"] = "Arnelify Server";
  }

  const void sendHeaders() {
    std::string response = "HTTP/1.1";
    response.append(" ");

    switch (this->code) {
      case 500:
        response.append("500 Internal Server Error");
        break;
      case 409:
        response.append("409 Conflict");
        break;
      case 404:
        response.append("404 Not Found");
        break;
      case 403:
        response.append("403 Forbidden");
        break;
      case 401:
        response.append("401 Unauthorized");
        break;
      case 206:
        response.append("206 Partial");
        break;
      default:
        response.append("200 OK");
        break;
    }

    response.append(" \r\n");
    for (const auto &pair : this->headers) {
      response.append(pair.first + ": " + pair.second + "\r\n");
    }

    response.append("\r\n");
    send(this->socket, response.c_str(), response.length(), 0);
    this->resetHeaders();
  }

  void sendBody(const std::size_t &bytesRead) {
    this->headers["Content-Length"] = std::to_string(bytesRead);
    this->sendHeaders();

    send(this->socket, this->body.c_str(), bytesRead, 0);
    this->body.clear();
  }

  void sendBodyCompressed(const std::size_t &bytesRead) {
    std::size_t bytesCompressed = 0;
    unsigned char *compressed = new unsigned char[this->blockSize];
    this->gzip(this->body.c_str(), bytesRead, compressed, bytesCompressed);
    this->body.clear();

    this->headers["Content-Encoding"] = "gzip";
    this->headers["Content-Length"] = std::to_string(bytesCompressed);
    this->sendHeaders();

    send(this->socket, compressed, bytesCompressed, 0);
    delete[] compressed;
  }

  void sendFile(std::ifstream &file, const std::size_t &fileSize) {
    this->headers["Content-Length"] = std::to_string(fileSize);
    this->sendHeaders();

    char *block = new char[this->blockSize];
    while (file.read(block, this->blockSize) || file.gcount() > 0) {
      const std::size_t bytesRead = file.gcount();
      send(this->socket, block, bytesRead, 0);
    }

    delete[] block;
  }

  void sendFileCompressed(std::ifstream &file) {
    char *block = new char[this->blockSize];
    file.read(block, this->blockSize);

    std::size_t bytesCompressed = 0;
    unsigned char *compressed = new unsigned char[this->blockSize];
    this->gzip(block, file.gcount(), compressed, bytesCompressed);
    delete[] block;

    this->headers["Content-Encoding"] = "gzip";
    this->headers["Content-Length"] = std::to_string(bytesCompressed);
    this->sendHeaders();

    send(this->socket, compressed, bytesCompressed, 0);
    delete[] compressed;
  }

 public:
  ArnelifyTransmitter(const int &s, ArnelifyTransmitterOpts &o)
      : blockSize(65536),
        code(200),
        isGzip(false),
        isStatic(false),
        opts(o),
        socket(s) {
    this->blockSize = this->opts.TRANSMITTER_BLOCK_SIZE_KB * 1024;
    this->resetHeaders(true);
  }

  void addBody(const std::string &body) {
    const bool hasFile = !this->filePath.empty();
    if (hasFile) {
      this->callback("Can't add body to a Response that contains a file.",
                     true);
      exit(1);
    }

    this->body.append(body);
  }

  void end() {
    const bool hasFile = !this->filePath.empty();
    if (hasFile) {
      this->body.clear();
      std::ifstream file(this->filePath, std::ios::binary);
      if (file.is_open()) {
        const std::size_t fileSize = std::filesystem::file_size(this->filePath);
        const std::string fileExt = filePath.extension().string();
        const std::string fileName = filePath.filename().string();
        this->headers["Content-Type"] = getMime(fileExt);
        if (!this->isStatic) {
          this->headers["Content-Disposition"] =
              "attachment; filename=\"" + fileName + "\"";
        }

        if (this->isGzip && this->blockSize > fileSize && fileSize > 96) {
          this->sendFileCompressed(file);
          return;
        }

        this->sendFile(file, fileSize);
        return;
      }

      this->code = 404;
      this->body = "{\"code\":404,\"Not found.\"}";
      this->callback("Failed to open file: " + std::string(this->filePath),
                     true);
    }

    const std::size_t bytesRead = this->body.length();
    if (this->isGzip && this->blockSize > bytesRead && bytesRead > 96) {
      this->sendBodyCompressed(bytesRead);
      return;
    }

    this->sendBody(bytesRead);
  }

  void setCallback(const ArnelifyTransmitterCallback &callback) {
    this->callback = callback;
  }

  void setCode(const int &code) { this->code = code; }

  void setEncoding(const std::string &encoding) {
    const std::size_t gzipStart = encoding.find("gzip");
    if (this->opts.TRANSMITTER_GZIP) {
      this->isGzip = gzipStart != std::string::npos;
    }
  }

  void setFile(const std::filesystem::path &filePath,
               const bool &isStatic = false) {
    const bool hasBody = !body.empty();
    if (hasBody) {
      this->callback(
          "Can't add an attachment to a Response that contains a body.", true);
      exit(1);
    }

    this->filePath = filePath;
    this->isStatic = isStatic;
  }

  void setHeader(const std::string &key, const std::string &value) {
    this->headers[key] = value;
  }
};

#endif