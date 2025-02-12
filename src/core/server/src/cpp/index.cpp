#ifndef ARNELIFY_SERVER_CPP
#define ARNELIFY_SERVER_CPP

#include <arpa/inet.h>
#include <functional>
#include <iostream>
#include <thread>
#include <unistd.h>

#include "receiver/index.cpp"
#include "transmitter/index.cpp"

#include "contracts/opts.hpp"

using ArnelifyServerRes = ArnelifyTransmitter *;
using ArnelifyServerCallback =
    std::function<void(const std::string &, const bool &)>;
using ArnelifyServerHandler =
    std::function<void(const ArnelifyServerReq &, ArnelifyServerRes)>;

class ArnelifyServer {
 private:
  const ArnelifyServerOpts opts;
  ArnelifyServerCallback callback = [](const std::string &message,
                                       const bool &isError) -> void {
    if (isError) {
      std::cout << "[Arnelify Server]: Error: " << message << std::endl;
      return;
    }

    std::cout << "[Arnelify Server]: " << message << std::endl;
  };

  ArnelifyServerHandler handler = [](const ArnelifyServerReq &req,
                                     ArnelifyServerRes res) -> void {
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    writer["emitUTF8"] = true;

    Json::Value json;
    json["code"] = 200;
    json["success"] = "Welcome to Arnelify Server";
    res->addBody(Json::writeString(writer, json));
    res->end();
  };

  bool isRunning;
  sockaddr_in serverAddr;
  int serverSocket;

  void read(const int &clientSocket, sockaddr_in &clientAddr) {
    char client[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, client, INET_ADDRSTRLEN);
    const std::size_t BLOCK_SIZE = this->opts.SERVER_BLOCK_SIZE_KB * 1024;
    const std::string SERVER_CLIENT = std::string(client);
    ArnelifyTransmitterOpts transmitterOpts(
        this->opts.SERVER_BLOCK_SIZE_KB, this->opts.SERVER_CHARSET,
        this->opts.SERVER_GZIP, SERVER_CLIENT);
    ArnelifyTransmitter *res =
        new ArnelifyTransmitter(clientSocket, transmitterOpts);
    ArnelifyReceiverOpts opts(
        this->opts.SERVER_ALLOW_EMPTY_FILES, SERVER_CLIENT,
        this->opts.SERVER_KEEP_EXTENSIONS, this->opts.SERVER_MAX_FIELDS,
        this->opts.SERVER_MAX_FIELDS_SIZE_TOTAL_MB, this->opts.SERVER_MAX_FILES,
        this->opts.SERVER_MAX_FILES_SIZE_TOTAL_MB,
        this->opts.SERVER_MAX_FILE_SIZE_MB, this->opts.SERVER_UPLOAD_DIR);
    ArnelifyReceiver *receiver = new ArnelifyReceiver(opts);

    ssize_t bytesRead = 0;
    char *block = new char[BLOCK_SIZE];
    int SIGNAL_ON_BLOCK = 0;
    while ((bytesRead = recv(clientSocket, block, BLOCK_SIZE, 0)) > 0) {
      SIGNAL_ON_BLOCK = receiver->onBlock(block, bytesRead);
      if (SIGNAL_ON_BLOCK > 0) break;
    }

    delete[] block;
    const bool isOnBlockError = SIGNAL_ON_BLOCK != 2;
    if (isOnBlockError) {
      Json::StreamWriterBuilder writer;
      writer["indentation"] = "";
      writer["emitUTF8"] = true;

      Json::Value json;
      json["code"] = 409;
      json["error"] = receiver->getStatus();
      const std::string body = Json::writeString(writer, json);

      res->setCode(409);
      res->addBody(body);
      res->end();

      delete receiver;
      delete res;
      return;
    }

    res->setCallback(this->callback);
    res->setEncoding(receiver->getEncoding());
    const ArnelifyServerReq req = receiver->finish();
    delete receiver;

    this->handler(req, res);
    close(clientSocket);
    delete res;
  }

  void acceptor() {
    sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    std::string sPort = std::to_string(this->opts.SERVER_PORT);
    this->callback("Server is running on port " + sPort, false);

    while (true) {
      bool isStop = !this->isRunning;
      if (isStop) {
        close(this->serverSocket);
        this->callback("Server stopped.", false);
        exit(0);
      }

      const int clientSocket =
          accept(this->serverSocket, (sockaddr *)&clientAddr, &clientLen);
      const bool isAcceptSuccess = clientSocket != -1;
      if (!isAcceptSuccess) {
        callback("Accept failed.", true);
        exit(1);
      }

      std::thread session([this, clientSocket, &clientAddr]() {
        this->read(clientSocket, clientAddr);
      });

      session.detach();
    }
  }

 public:
  ArnelifyServer(ArnelifyServerOpts &o) : isRunning(false), opts(o) {}
  void setHandler(const ArnelifyServerHandler &handler) {
    this->handler = handler;
  }

  void start(const ArnelifyServerCallback &callback) {
    const std::filesystem::path uploadDir = this->opts.SERVER_UPLOAD_DIR;
    const bool hasUploadDir = std::filesystem::exists(uploadDir);
    if (!hasUploadDir) std::filesystem::create_directory(uploadDir);
    this->callback = callback;
    this->isRunning = true;

    this->serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    const bool isServerSocketCreated = this->serverSocket != -1;
    if (!isServerSocketCreated) {
      this->callback("Socket creation failed.", true);
      exit(1);
    }

    this->serverAddr.sin_family = AF_INET;
    this->serverAddr.sin_addr.s_addr = INADDR_ANY;
    this->serverAddr.sin_port = htons(this->opts.SERVER_PORT);

    int opt = 1;
    setsockopt(this->serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    const bool isBindSuccess =
        bind(this->serverSocket, (sockaddr *)&this->serverAddr,
             sizeof(this->serverAddr)) != -1;
    if (!isBindSuccess) {
      this->callback("Bind failed.", true);
      close(this->serverSocket);
      exit(1);
    }

    const bool isListenSuccess =
        listen(this->serverSocket, this->opts.SERVER_QUEUE_LIMIT) != -1;
    if (!isListenSuccess) {
      this->callback("Listen failed.", true);
      close(this->serverSocket);
      exit(1);
    }

    this->acceptor();
    close(this->serverSocket);
  }

  void stop() { this->isRunning = false; }
};

#endif