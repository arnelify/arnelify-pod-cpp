#ifndef ARNELIFY_ROUTER_ROUTE_HPP
#define ARNELIFY_ROUTER_ROUTE_HPP

#include "json.h"

struct Route {
  int id;
  std::optional<std::string> method;
  Json::Value params;
  std::string pattern;

  /* It's important that the arguments aren't references */
  Route(const int i, const std::optional<std::string> m, const Json::Value par,
        const std::string pat)
      : id(i), method(m), params(par), pattern(pat) {}
};

using RouterCallback = std::function<void(const std::optional<Route>&)>;

#endif