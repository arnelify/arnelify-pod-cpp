#ifndef ARNELIFY_ROUTER_CPP
#define ARNELIFY_ROUTER_CPP

#include <dlfcn.h>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>

#include "json.h"

#include "cpp/contracts/controller.hpp"
#include "cpp/contracts/route.hpp"

class ArnelifyRouter {
 private:
  void* lib = nullptr;
  std::filesystem::path libPath;
  std::map<int, Controller> controllers;
  int iterator;

  void (*router_create)();
  void (*router_destroy)();
  void (*router_any)(const char*);
  void (*router_get)(const char*);
  void (*router_post)(const char*);
  void (*router_put)(const char*);
  void (*router_patch)(const char*);
  void (*router_delete)(const char*);
  const char* (*router_find)(const char*, const char*);
  void (*router_free)(const char*);
  void (*router_reset)();

  template <typename T>
  void loadFunction(const std::string& name, T& func) {
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
    for (const auto& segment : segments) {
      libPath += "/" + segment;
    }

    libPath += "/build/index.so";
    return libPath;
  }

 public:
  ArnelifyRouter() : iterator(0) {
    const std::string libPath = this->getLibPath();
    this->lib = dlopen(libPath.c_str(), RTLD_LAZY);
    if (!this->lib) throw std::runtime_error(dlerror());

    loadFunction("router_create", this->router_create);
    loadFunction("router_destroy", this->router_destroy);
    loadFunction("router_any", this->router_any);
    loadFunction("router_get", this->router_get);
    loadFunction("router_post", this->router_post);
    loadFunction("router_put", this->router_put);
    loadFunction("router_patch", this->router_patch);
    loadFunction("router_delete", this->router_delete);
    loadFunction("router_find", this->router_find);
    loadFunction("router_free", this->router_free);
    loadFunction("router_reset", this->router_reset);

    this->router_create();
  }

  ~ArnelifyRouter() {
    if (!this->lib) return;
    this->router_destroy();
    dlclose(this->lib);
    this->lib = nullptr;
  }

  void any(const std::string& pattern, const Controller& controller) {
    const int id = this->iterator;
    this->controllers[id] = controller;
    this->iterator++;

    this->router_any(pattern.c_str());
  }

  void get(const std::string& pattern, const Controller& controller) {
    const int id = this->iterator;
    this->controllers[id] = controller;
    this->iterator++;

    this->router_get(pattern.c_str());
  }

  void post(const std::string& pattern, const Controller& controller) {
    const int id = this->iterator;
    this->controllers[id] = controller;
    this->iterator++;

    this->router_get(pattern.c_str());
  }

  void put(const std::string& pattern, const Controller& controller) {
    const int id = this->iterator;
    this->controllers[id] = controller;
    this->iterator++;

    this->router_get(pattern.c_str());
  }

  void patch(const std::string& pattern, const Controller& controller) {
    const int id = this->iterator;
    this->controllers[id] = controller;
    this->iterator++;

    this->router_get(pattern.c_str());
  }

  void delete_(const std::string& pattern, const Controller& controller) {
    const int id = this->iterator;
    this->controllers[id] = controller;
    this->iterator++;

    this->router_get(pattern.c_str());
  }

  std::optional<Route> find(const std::string& method,
                            const std::string& path) {
    const char* cRouteOpt = this->router_find(method.c_str(), path.c_str());

    Json::Value routeOpt;
    Json::CharReaderBuilder reader;
    std::string errors;

    std::istringstream iss(cRouteOpt);
    if (!Json::parseFromStream(reader, iss, &routeOpt, &errors)) {
      std::cout << "[ArnelifyRouter FFI]: C error: Invalid cRouteOpt."
                << std::endl;
      exit(1);
    }

    this->router_free(cRouteOpt);

    if (!routeOpt.isMember("id")) return std::nullopt;

    std::optional<std::string> methodOpt;
    if (routeOpt.isMember("method") && !routeOpt["method"].isNull()) {
      methodOpt = routeOpt["method"].asString();
    }

    Route route(routeOpt["id"].asInt(), methodOpt, routeOpt["params"],
                routeOpt["pattern"].asString());
    return route;
  }

  const std::optional<Controller> getController(const int& id) {
    auto it = this->controllers.find(id);
    const bool hasId = it != this->controllers.end();
    if (hasId) return it->second;
    return std::nullopt;
  }

  const void reset() { this->router_reset(); }
};

#endif