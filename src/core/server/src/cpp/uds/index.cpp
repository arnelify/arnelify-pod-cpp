#ifndef ARNELIFY_UNIX_DOMAIN_SOCKET_CLIENT_CPP
#define ARNELIFY_UNIX_DOMAIN_SOCKET_CLIENT_CPP

#include <arpa/inet.h>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <random>
#include <sys/socket.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>

#include "json.h"

#include "contracts/opts.hpp"

class ArnelifyUnixDomainSocketClient {
 private:
  std::string buffer;
  std::function<void(const std::string&, const bool&)> callback =
      [](const std::string& message, const bool& isError) {
        if (isError) std::cout << "Error: " << message << std::endl;
      };

  int clientSocket;
  const ArnelifyUnixDomainSocketClientOpts opts;
  std::map<std::string, std::function<void(const std::string&)>> res;
  int size;

  void read() {
    std::thread thread([this]() {
      const int BLOCK_SIZE = this->opts.UDS_BLOCK_SIZE_KB * 1024;

      ssize_t bytesRead = 0;
      char block[BLOCK_SIZE];
      while ((bytesRead = recv(this->clientSocket, block, BLOCK_SIZE, 0)) > 0) {
        this->receiver(block, bytesRead);
      }
    });

    thread.detach();
  }

  void receiver(const char* block, const std::size_t bytesRead) {
    this->buffer.append(block, bytesRead);

    while (this->buffer.length() > 0) {
      if (!this->size) {
        const std::size_t sizeEnd = this->buffer.find(":");
        const bool hasColon = sizeEnd != std::string::npos;
        if (hasColon) {
          this->size = std::stoi(this->buffer.substr(0, sizeEnd));
          this->buffer = this->buffer.substr(sizeEnd + 1);
        }
      }

      if (this->size > this->buffer.length()) break;
      if (this->buffer.length() >= this->size) {
        const std::string message = this->buffer.substr(0, this->size);

        Json::Value json;
        Json::CharReaderBuilder reader;
        std::string errors;
        std::istringstream iss(message);
        if (!Json::parseFromStream(reader, iss, &json, &errors)) {
          this->callback(
              "Message from UDS (Unix Domain Socket) must be in valid JSON "
              "format.",
              true);
          close(this->clientSocket);
          return;
        }
        
        const std::string uuid = json["uuid"].asString();
        const std::function<void(const std::string&)> resolve = this->res[uuid];

        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";
        writer["emitUTF8"] = true;
        resolve(Json::writeString(writer, json["content"]));
        this->res.erase(uuid);

        this->buffer = this->buffer.substr(this->size);
        this->size = 0;
      }
    }
  }

 public:
  ArnelifyUnixDomainSocketClient(const ArnelifyUnixDomainSocketClientOpts& o)
      : clientSocket(0), opts(o), size(0) {}

  void connect(
      const std::function<void(const std::string&, const bool&)>& callback) {
    this->callback = callback;

    const std::string socketPath = this->opts.UDS_SOCKET_PATH;
    const bool hasSocket = std::filesystem::exists(socketPath);
    if (!hasSocket) {
      this->callback("Error connecting to UDS (Unix Domain Socket).", true);
      return;
    }

    this->clientSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    const bool isServerSocketCreated = this->clientSocket == -1;
    if (isServerSocketCreated) {
      this->callback("Error connecting to UDS (Unix Domain Socket).", true);
      return;
    }

    sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;

    strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);
    const bool isConnected =
        ::connect(this->clientSocket, (struct sockaddr*)&addr, sizeof(addr));
    if (isConnected == -1) {
      this->callback("Error connecting to UDS (Unix Domain Socket).", true);
      close(this->clientSocket);
      return;
    }

    this->read();
  }

  const std::string createUuId() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(10000, 19999);
    int random = dis(gen);
    const auto now = std::chrono::system_clock::now();
    const auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch())
            .count();

    const std::string code =
        std::to_string(milliseconds) + std::to_string(random);
    std::hash<std::string> hasher;
    size_t v1 = hasher(code);
    size_t v2 = hasher(std::to_string(v1));
    unsigned char hash[16];
    for (int i = 0; i < 8; ++i) {
      hash[i] = (v1 >> (i * 8)) & 0xFF;
      hash[i + 8] = (v2 >> (i * 8)) & 0xFF;
    }

    std::stringstream ss;
    for (int i = 0; i < 16; ++i) {
      ss << std::hex << std::setw(2) << std::setfill('0')
         << static_cast<int>(hash[i]);
    }

    return ss.str();
  }

  void on(const std::string& requestId,
          const std::function<void(const std::string&)>& onMessage) {
    this->res[requestId] = onMessage;
  }

  void write(const std::string& content) {
    const std::string message =
        std::to_string(content.length()) + ":" + content;
    if (send(this->clientSocket, message.c_str(), message.length(), 0) == -1) {
      this->callback("Failed to send message to UDS (Unix Domain Socket).",
                     true);
      close(this->clientSocket);
      return;
    }
  }
};

#endif