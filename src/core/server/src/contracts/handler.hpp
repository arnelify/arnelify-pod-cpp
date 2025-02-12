#ifndef ARNELIFY_SERVER_HANDLER_HPP
#define ARNELIFY_SERVER_HANDLER_HPP

#include <filesystem>
#include <functional>
#include <iostream>

#include "json.h"

using ArnelifyServerCallback =
    std::function<void(const std::string&, const bool&)>;
using ArnelifyServerReq = Json::Value;

struct ArnelifyServerRes {
 private:
  Json::Value res;
  ArnelifyServerCallback callback = [](const std::string& message,
                                       const bool& isError) {
    if (isError) {
      std::cout << "[Arnelify Server]: Error: " << message << std::endl;
      exit(1);
    }

    std::cout << "[Arnelify Server]: " << message << std::endl;
  };

 public:
  ArnelifyServerRes() {
    this->res["body"] = "";
    this->res["code"] = 200;
    this->res["filePath"] = "";
    this->res["headers"] = Json::objectValue;
  }

  void addBody(const std::string& chunk) {
    const std::string filePath = this->res["filePath"].asString();
    const bool hasFile = !filePath.empty();
    if (hasFile) {
      this->callback("Can't add body to a Response that contains a file.",
                     true);
      exit(1);
    }

    this->res["body"] = this->res["body"].asString() + chunk;
  }

  void end() {
    const std::string filePath = this->res["filePath"].asString();
    const bool hasFile = !filePath.empty();
    if (hasFile) {
      this->res["body"] = "";
      return;
    }

    const std::string body = this->res["body"].asString();
    const bool hasBody = !body.empty();
    if (hasBody) {
      this->res["filePath"] = "";
      this->res["isStatic"] = false;
      return;
    }

    this->callback("Add the body or set the file.", true);
    exit(1);
  }

  void setCallback(const ArnelifyServerCallback& callback) {
    this->callback = callback;
  }

  void setCode(const int& code) { this->res["code"] = code; }

  void setFile(const std::string& filePath, const bool& isStatic = false) {
    const std::string body = this->res["body"].asString();
    const bool hasBody = !body.empty();
    if (hasBody) {
      this->callback(
          "Can't add an attachment to a Response that contains a body.", true);
      return;
    }

    this->res["filePath"] = filePath;
    this->res["isStatic"] = isStatic;
  }

  void setHeader(const std::string& key, const std::string& value) {
    this->res["headers"][key] = value;
  }

  const std::string serialize() {
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";

    return Json::writeString(writer, this->res);
  }
};

using ArnelifyServerHandler =
    std::function<void(const ArnelifyServerReq&, ArnelifyServerRes&)>;

struct StdToC {
  static std::function<void(const std::string&, const bool&)> callback;
  static ArnelifyServerHandler handler;

  void setStdCallback(const ArnelifyServerCallback& callback) {
    StdToC::callback = callback;
  }

  void setStdHandler(const ArnelifyServerHandler& handler) {
    StdToC::handler = handler;
  }

  static void cCallback(const char* cMessage, const int isError) {
    callback(cMessage, isError);
  };

  static const char* cHandler(const char* cSerialized) {
    Json::Value req;
    Json::CharReaderBuilder reader;
    std::string errors;
    std::istringstream iss(cSerialized);
    if (!Json::parseFromStream(reader, iss, &req, &errors)) {
      std::cout
          << "[ArnelifyServer FFI]: Error in C++: cReq must be a valid JSON."
          << std::endl;
      exit(1);
    }

    ArnelifyServerRes res;
    handler(req, res);
    const std::string serialized = res.serialize();

    char* cRes = new char[serialized.size() + 1];
    std::copy(serialized.begin(), serialized.end(), cRes);
    cRes[serialized.size()] = '\0';
    return cRes;
  }
};

std::function<void(const std::string&, const bool&)> StdToC::callback = nullptr;
ArnelifyServerHandler StdToC::handler = nullptr;

#endif