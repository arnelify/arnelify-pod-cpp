#ifndef ARNELIFY_ROUTER_TEST_CPP
#define ARNELIFY_ROUTER_TEST_CPP

#include <iostream>

#include "json.h"

#include "../index.cpp"

int main(int argc, char* argv[]) {
  ArnelifyRouter router;

  router.get("/", [](const Ctx& ctx) -> Json::Value { return ctx; });
  router.get("/:id", [](const Ctx& ctx) -> Json::Value { return ctx; });

  const std::optional<Route> routeOpt = router.find("GET", "/1");
  if (!routeOpt) {
    std::cout << "[Arnelify Router]: Route not found." << std::endl;
    return 0;
  }

  const Route route = *routeOpt;
  Json::Value serialized;
  serialized["id"] = route.id;
  if (route.method) {
    serialized["method"] = *route.method;
  } else {
    serialized["method"] = Json::nullValue;
  }

  serialized["params"] = route.params;
  serialized["pattern"] = route.pattern;

  Json::StreamWriterBuilder writer;
  writer["indentation"] = "";
  writer["emitUTF8"] = true;

  std::cout << "[Arnelify Router]: Route serialized: "
            << Json::writeString(writer, serialized) << std::endl;

  const std::optional<Controller> controllerOpt =
      router.getController(route.id);
  if (!controllerOpt) {
    std::cout << "[Arnelify Router]: Controller not found." << std::endl;
    return 1;
  }

  const Controller controller = *controllerOpt;

  Json::Value ctx;
  ctx["code"] = 200;
  ctx["success"] = "Welcome to Arnelify Router";

  const Json::Value res = controller(ctx);
  std::cout << "[Arnelify Router]: Response: " << Json::writeString(writer, res)
            << std::endl;
  return 0;
}

#endif