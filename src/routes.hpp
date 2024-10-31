#ifndef ROUTES_HPP
#define ROUTES_HPP

#include "router/index.hpp"
#include "broker/index.hpp"

#include "middleware/test.hpp"
#include "services/first.hpp"
#include "services/second.hpp"

void routes(Router& router) {
  broker.subscribe("first.welcome", [](const BrokerCtx& ctx) {
    First first;
    BrokerCtx newCtx = testMiddleware(ctx);
    return first.welcome(newCtx);
  });

  broker.subscribe("second.welcome", [](const BrokerCtx& ctx) {
    Second second;
    return second.welcome(ctx);
  });

  router.get("/", [](const RouterCtx& ctx) -> Json::Value {
    return broker.call("first.welcome", ctx["params"]);
  });
}

#endif