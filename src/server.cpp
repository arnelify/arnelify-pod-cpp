#ifndef BUILD_SERVER_CPP
#define BUILD_SERVER_CPP

#include <iostream>
#include <optional>

#include "json.h"

#include "core/env/index.cpp"
#include "core/logger/index.cpp"
#include "core/server/index.cpp"

#include "core/broker/index.cpp"
#include "core/router/index.cpp"
#include "routes.cpp"

#include "server/src/cpp/contracts/opts.hpp"

int main(int argc, char* argv[]) {
  ArnelifyRouter router;
  routes(router);

  ArnelifyServerOpts opts(
  env.SERVER_ALLOW_EMPTY_FILES == "true",
  std::stoi(env.SERVER_BLOCK_SIZE_KB),
  env.SERVER_CHARSET,
  env.SERVER_GZIP == "true",
  env.SERVER_KEEP_EXTENSIONS == "true",
  std::stoi(env.SERVER_MAX_FIELDS),
  std::stoi(env.SERVER_MAX_FIELDS_SIZE_TOTAL_MB),
  std::stoi(env.SERVER_MAX_FILES),
  std::stoi(env.SERVER_MAX_FILES_SIZE_TOTAL_MB),
  std::stoi(env.SERVER_MAX_FILE_SIZE_MB),
  std::stoi(env.SERVER_PORT),
  std::stoi(env.SERVER_QUEUE_LIMIT),
  "./src/storage/upload");

  ArnelifyServer server(opts);

  server.setHandler(
      [&router](const ArnelifyServerReq& req, ArnelifyServerRes res) {
        Json::StreamWriterBuilder writer;
        writer["indentation"] = "";
        writer["emitUTF8"] = true;

        const std::string method = req["_state"]["method"].asString();
        const std::string path = req["_state"]["path"].asString();
        const std::optional<Route> routeOpt = router.find(method, path);
        if (!routeOpt) {
          Json::Value json;
          json["code"] = 404;
          json["error"] = "Not found.";

          res->setCode(404);
          res->addBody(Json::writeString(writer, json));
          res->end();
          return;
        }

        res->setCode(200);
        const Route& route = *routeOpt;
        const std::optional<Controller> controllerOpt =
            router.getController(route.id);
        const Controller& controller = *controllerOpt;

        Ctx ctx;
        ctx["params"] = req;
        const Json::Value response = controller(ctx);
        const bool isObject = response.isObject();
        if (!isObject) {
          res->addBody(Json::writeString(writer, response));
          res->end();
          return;
        }

        const bool hasCode = response.isMember("code") && response.isInt();
        if (hasCode) res->setCode(response["code"].asInt());
        res->addBody(Json::writeString(writer, response));
        res->end();
      });

  server.start([](const std::string& message, const bool& isError) {
    if (isError) {
      Logger::danger(message + "\n");
      return;
    }

    Logger::success(message + "\n");
  });

  return 0;
}

#endif