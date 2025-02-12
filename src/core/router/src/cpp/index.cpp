#ifndef ARNELIFY_ROUTER_CPP
#define ARNELIFY_ROUTER_CPP

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>
#include <vector>

#include "contracts/controller.hpp"
#include "contracts/route.hpp"
#include "contracts/segments.hpp"

class ArnelifyRouter {
 private:
  int iterator;
  std::map<int, Controller> controllers;
  std::vector<Route> routes;

  const Segments split(const std::string& string,
                       const std::string& delimiter) {
    Segments segments;
    int start = 0;
    int end;

    while ((end = string.find(delimiter, start)) != std::string::npos) {
      segments.emplace_back(string.substr(start, end - start));
      start = end + delimiter.length();
    }

    segments.emplace_back(string.substr(start));
    return segments;
  }

  const int paramsLen(const Segments& segments) {
    auto condition = [segments](const std::string& segment) {
      return !segment.empty() && segment[0] == ':';
    };

    return std::count_if(segments.begin(), segments.end(), condition);
  };

  void sort() {
    std::sort(this->routes.begin(), this->routes.end(),
              [this](const Route& a, const Route& b) {
                const auto segmentsA = this->split(a.pattern, "/");
                const auto segmentsB = this->split(b.pattern, "/");
                const int paramsA = this->paramsLen(segmentsA);
                const int paramsB = this->paramsLen(segmentsB);
                const bool isParamsEqual = paramsA == paramsB;
                if (!isParamsEqual) return paramsA < paramsB;
                return segmentsA.size() < segmentsB.size();
              });
  }

  const std::optional<Route> compare(Route& route,
                                     const Segments& routeSegments,
                                     const Segments& pathSegments) {
    const bool isLenEqual = routeSegments.size() == pathSegments.size();
    if (!isLenEqual) return std::nullopt;

    for (std::size_t i = 0; routeSegments.size() > i; ++i) {
      bool hasParam = routeSegments[i].starts_with(':');
      if (hasParam) {
        const std::string key = routeSegments[i].substr(1);
        const std::string param = pathSegments[i];
        route.params[key] = param;
        continue;
      }

      const bool isMatch = routeSegments[i] == pathSegments[i];
      if (!isMatch) return std::nullopt;
    }

    return route;
  }

 public:
  ArnelifyRouter() : iterator(0) {}
  void any(const std::string& pattern, const Controller& controller) {
    const int id = this->iterator;
    this->controllers[id] = controller;
    this->iterator++;

    Json::Value params = Json::objectValue;
    Route route(id, std::nullopt, params, pattern);
    this->routes.emplace_back(route);
    this->sort();
  }

  void get(const std::string& pattern, const Controller& controller) {
    const int id = this->iterator;
    this->controllers[id] = controller;
    this->iterator++;

    Json::Value params = Json::objectValue;
    Route route(id, "GET", params, pattern);
    this->routes.emplace_back(route);
    this->sort();
  }

  void post(const std::string& pattern, const Controller& controller) {
    const int id = this->iterator;
    this->controllers[id] = controller;
    this->iterator++;

    Json::Value params = Json::objectValue;
    Route route(id, "POST", params, pattern);
    this->routes.emplace_back(route);
    this->sort();
  }

  void put(const std::string& pattern, const Controller& controller) {
    const int id = this->iterator;
    this->controllers[id] = controller;
    this->iterator++;

    Json::Value params = Json::objectValue;
    Route route(id, "PUT", params, pattern);
    this->routes.emplace_back(route);
    this->sort();
  }

  void patch(const std::string& pattern, const Controller& controller) {
    const int id = this->iterator;
    this->controllers[id] = controller;
    this->iterator++;

    Json::Value params = Json::objectValue;
    Route route(id, "PATCH", params, pattern);
    this->routes.emplace_back(route);
    this->sort();
  }

  void delete_(const std::string& pattern, const Controller& controller) {
    const int id = this->iterator;
    this->controllers[id] = controller;
    this->iterator++;

    Json::Value params = Json::objectValue;
    Route route(id, "DELETE", params, pattern);
    this->routes.emplace_back(route);
    this->sort();
  }

  const std::optional<Route> find(const std::string& method,
                                  const std::string& path) {
    const Segments pathSegments = this->split(path, "/");
    for (Route route : routes) {
      std::optional<Route> routeOpt;
      if (route.method && *route.method == method) {
        const Segments routeSegments = this->split(route.pattern, "/");
        routeOpt = this->compare(route, routeSegments, pathSegments);
        if (routeOpt) return routeOpt;
      } else {
        const Segments routeSegments = this->split(route.pattern, "/");
        routeOpt = this->compare(route, routeSegments, pathSegments);
        if (routeOpt) return routeOpt;
      }
    }

    return std::nullopt;
  }

  const std::optional<Controller> getController(const int& id) {
    auto it = this->controllers.find(id);
    const bool hasId = it != this->controllers.end();
    if (hasId) return it->second;
    return std::nullopt;
  }

  void reset() {
    this->controllers.clear();
    this->routes.clear();
  }
};

#endif