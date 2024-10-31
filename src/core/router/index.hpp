#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <algorithm>
#include <functional>
#include <iostream>
#include <vector>

#include "contracts/ctx.h"
#include "contracts/callback.h"
#include "contracts/controller.h"
#include "contracts/route.h"

class Router {
 private:
  std::vector<Route> routes;

  const std::vector<std::string> split(const std::string& string,
                                       const std::string& delimiter) {
    std::vector<std::string> parts;
    int start = 0;
    int end;

    while ((end = string.find(delimiter, start)) != std::string::npos) {
      parts.emplace_back(string.substr(start, end - start));
      start = end + delimiter.length();
    }

    parts.emplace_back(string.substr(start));

    return parts;
  }

  const int paramsLen(const std::vector<std::string>& segments) {
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
                                     const std::vector<std::string>& possible,
                                     const std::vector<std::string>& current) {
    bool isLenEqual = possible.size() == current.size();
    if (!isLenEqual) return std::nullopt;

    for (std::size_t i = 0; possible.size() > i; ++i) {
      bool hasParam = possible[i].starts_with(':');
      if (hasParam) {
        const std::string key = possible[i].substr(1);
        const std::string param = current[i];
        route.params[key] = param;

      } else {
        const bool isMatch = possible[i] == current[i];
        if (!isMatch) return std::nullopt;
      }
    }

    return route;
  }

  const std::optional<Route> find(const std::string& method,
                                  const std::string& path) {
    const std::vector<std::string> current = this->split(path, "/");

    for (const Route& item : routes) {
      const bool isAnyMethod = !item.method;
      const bool isExplicitMethod = *item.method == method;
      if (isAnyMethod || isExplicitMethod) {
        Json::Value params = Json::objectValue;
        Route route(item.controller, method, params, item.pattern);
        const std::vector<std::string> possible = this->split(item.pattern, "/");
        const std::optional<Route> isFound =
            this->compare(route, possible, current);
        if (isFound) return route;
      }
    }

    return std::nullopt;
  }

 public:
  void any(const std::string& pattern, Controller controller) {
    Json::Value params = Json::objectValue;
    Route newRoute(controller, std::nullopt, params, pattern);
    this->routes.emplace_back(newRoute);
    this->sort();
  }

  void get(const std::string& pattern, Controller controller) {
    Json::Value params = Json::objectValue;
    Route newRoute(controller, "GET", params, pattern);
    this->routes.emplace_back(newRoute);
    this->sort();
  }

  void post(const std::string& pattern, Controller controller) {
    Json::Value params = Json::objectValue;
    Route newRoute(controller, "POST", params, pattern);
    this->routes.emplace_back(newRoute);
    this->sort();
  }

  void put(const std::string& pattern, Controller controller) {
    Json::Value params = Json::objectValue;
    Route newRoute(controller, "PUT", params, pattern);
    this->routes.emplace_back(newRoute);
    this->sort();
  }

  void patch(const std::string& pattern, Controller controller) {
    Json::Value params = Json::objectValue;
    Route newRoute(controller, "PATCH", params, pattern);
    this->routes.emplace_back(newRoute);
    this->sort();
  }

  void delete_(const std::string& pattern, Controller controller) {
    Json::Value params = Json::objectValue;
    Route newRoute(controller, "DELETE", params, pattern);
    this->routes.emplace_back(newRoute);
    this->sort();
  }

  const void request(const RouterCtx& ctx, RouterCallback callback) {
    const std::string method = ctx["params"]["_state"]["method"].asString();
    const std::string path = ctx["params"]["_state"]["path"].asString();
    const std::optional<Route> routeOpt = this->find(method, path);
    callback(routeOpt);
  }

  void reset() { this->routes.clear(); }
};

#endif