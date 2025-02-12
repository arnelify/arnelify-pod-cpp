#ifndef ROUTES_CPP
#define ROUTES_CPP

#include "broker/src/index.cpp"
#include "router/src/index.cpp"

#include "middleware/test.cpp"
#include "services/first.cpp"
#include "services/second.cpp"

void routes(ArnelifyRouter& router) {
  broker->subscribe("second.welcome", [](const Ctx& ctx) -> Json::Value {
    Second second;
    return second.welcome(ctx);
  });
  
  broker->subscribe("first.welcome", [](const Ctx& ctx) -> Json::Value {
    First first;
    Ctx newCtx = testMiddleware(ctx);
    return first.welcome(newCtx);
  });

  router.get("/", [](const Ctx& ctx) -> Json::Value {
    return broker->call("first.welcome", ctx["params"]);
  });
}

#endif