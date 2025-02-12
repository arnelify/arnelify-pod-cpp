#ifndef WATCH_SERVER_CPP
#define WATCH_SERVER_CPP

#include <iostream>
#include <optional>

#include "json.h"

#include "env/index.cpp"
#include "logger/index.cpp"
#include "server/index.cpp"

#include "broker/index.cpp"
#include "router/index.cpp"
#include "routes.cpp"

int main(int argc, char* argv[]) {
  ArnelifyRouter router;
  routes(router);

  Json::Value opts;
  opts["SERVER_ALLOW_EMPTY_FILES"] = env.SERVER_ALLOW_EMPTY_FILES == "true";
  opts["SERVER_BLOCK_SIZE_KB"] = std::stoi(env.SERVER_BLOCK_SIZE_KB);
  opts["SERVER_CHARSET"] = env.SERVER_CHARSET;
  opts["SERVER_GZIP"] = env.SERVER_GZIP == "true";
  opts["SERVER_KEEP_EXTENSIONS"] = env.SERVER_KEEP_EXTENSIONS == "true";
  opts["SERVER_MAX_FIELDS"] = std::stoi(env.SERVER_MAX_FIELDS);
  opts["SERVER_MAX_FIELDS_SIZE_TOTAL_MB"] =
      std::stoi(env.SERVER_MAX_FIELDS_SIZE_TOTAL_MB);
  opts["SERVER_MAX_FILES"] = std::stoi(env.SERVER_MAX_FILES);
  opts["SERVER_MAX_FILES_SIZE_TOTAL_MB"] =
      std::stoi(env.SERVER_MAX_FILES_SIZE_TOTAL_MB);
  opts["SERVER_MAX_FILE_SIZE_MB"] = std::stoi(env.SERVER_MAX_FILE_SIZE_MB);
  opts["SERVER_PORT"] = std::stoi(env.SERVER_PORT);
  opts["SERVER_QUEUE_LIMIT"] = std::stoi(env.SERVER_QUEUE_LIMIT);
  opts["SERVER_UPLOAD_PATH"] = "./src/storage/upload";
  ArnelifyServer server(opts);

  server.setHandler(
      [&router](const ArnelifyServerReq& req, ArnelifyServerRes& res) {
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

          res.setCode(404);
          res.addBody(Json::writeString(writer, json));
          res.end();
          return;
        }

        res.setCode(200);
        const Route& route = *routeOpt;
        const std::optional<Controller> controllerOpt =
            router.getController(route.id);
        const Controller& controller = *controllerOpt;

        Ctx ctx;
        ctx["params"] = req;
        const Json::Value response = controller(ctx);
        const bool isObject = response.isObject();
        if (!isObject) {
          res.addBody(Json::writeString(writer, response));
          res.end();
          return;
        }

        const bool hasCode = response.isMember("code") && response.isInt();
        if (hasCode) res.setCode(response["code"].asInt());
        res.addBody(Json::writeString(writer, response));
        res.end();
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