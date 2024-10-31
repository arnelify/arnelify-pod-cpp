#ifndef ROUTER_ROUTE_CONTRACT_H
#define ROUTER_ROUTE_CONTRACT_H

#include <any>
#include <functional>
#include <iostream>
#include <map>

#include "json.h"

#include "controller.h"

struct Route {
  Controller controller;
  std::optional<std::string> method;
  Json::Value params;
  std::string pattern;

  Route(const Controller &c, const std::optional<std::string> &m,
        const Json::Value &par, const std::string &pat)
      : controller(c), method(m), params(par), pattern(pat) {}
};

#endif