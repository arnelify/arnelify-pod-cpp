#ifndef ARNELIFY_SERVER_CPP
#define ARNELIFY_SERVER_CPP

#include <dlfcn.h>
#include <string>
#include <filesystem>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "json.h"

#include "contracts/handler.hpp"

class ArnelifyServer {
 private:
  void *lib = nullptr;
  const Json::Value opts;
  StdToC stdtoc;

  void (*server_create)(const char *);
  void (*server_destroy)();
  void (*server_set_handler)(const char *(*)(const char *), const int);
  void (*server_start)(void (*)(const char *, const int));
  void (*server_stop)();

  template <typename T>
  void loadFunction(const std::string &name, T &func) {
    func = reinterpret_cast<T>(dlsym(this->lib, name.c_str()));
    if (!func) {
      throw std::runtime_error(dlerror());
    }
  }

  const std::string getLibPath() {
    const std::filesystem::path scriptDir =
        std::filesystem::absolute(__FILE__).parent_path();

    const bool hasExitSegment = scriptDir.string().ends_with("..");
    if (!hasExitSegment) {
      std::filesystem::path libDir = scriptDir.parent_path();
      const std::string libPath = libDir / "build" / "index.so";
      return libPath;
    }

    std::istringstream stream(scriptDir);
    std::deque<std::string> segments;
    std::string segment;

    while (std::getline(stream, segment, '/')) {
      if (segment.empty()) continue;

      if (segment == "..") {
        if (!segments.empty()) {
          segments.pop_back();
          segments.pop_back();
        }
        continue;
      }

      segments.push_back(segment);
    }

    std::string libPath;
    for (const auto &segment : segments) {
      libPath += "/" + segment;
    }

    libPath += "/build/index.so";
    return libPath;
  }

  ArnelifyServerHandler handler = [](const ArnelifyServerReq &req,
                                     ArnelifyServerRes &res) {
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    writer["emitUTF8"] = true;

    Json::Value json;
    json["code"] = 200;
    json["success"] = "Welcome to Arnelify Server";
    res.addBody(Json::writeString(writer, json));
    res.end();
  };

 public:
  ArnelifyServer(const Json::Value &o) : opts(o) {
    const std::string libPath = this->getLibPath();
    this->lib = dlopen(libPath.c_str(), RTLD_LAZY);
    if (!this->lib) throw std::runtime_error(dlerror());

    loadFunction("server_create", this->server_create);
    loadFunction("server_destroy", this->server_destroy);
    loadFunction("server_set_handler", this->server_set_handler);
    loadFunction("server_start", this->server_start);
    loadFunction("server_stop", this->server_stop);

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    writer["emitUTF8"] = true;

    const std::string cOpts = Json::writeString(writer, this->opts);
    this->server_create(cOpts.c_str());
  }

  ~ArnelifyServer() {
    if (!this->lib) return;
    this->server_destroy();
    dlclose(this->lib);
    this->lib = nullptr;
  }

  void setHandler(const ArnelifyServerHandler &handler) {
    stdtoc.setStdHandler(handler);
    this->server_set_handler(StdToC::cHandler, 1);
  }

  void start(const ArnelifyServerCallback &callback) {
    stdtoc.setStdCallback(callback);
    this->server_start(StdToC::cCallback);
  }

  void stop() { this->server_stop(); }
};

#endif