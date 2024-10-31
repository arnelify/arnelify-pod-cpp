#include <any>

#include "json.h"

#include "env/index.hpp"
#include "core/boot/boost/index.hpp"
#include "logger/index.hpp"

#include "router/index.hpp"
#include "routes.hpp"

class Server {
 private:
  Router router;
  BoostServer server;
  std::any io;

  RouterCtx getCtx(const BoostReq& req) {
    Json::Value _state = Json::objectValue;
    _state["agent"] = req["user_agent"];
    _state["cookie"] = req["cookie"];
    _state["host"] = req["host"];
    _state["ip"] = req["clientIP"];
    _state["method"] = req["method"];
    _state["path"] = req["target"];
    _state["route"] = Json::nullValue;

    Json::Value params = Json::objectValue;
    params["_state"] = _state;

    const bool hasParams = req["params"].isObject() && req.isMember("params");
    if (hasParams) {
      for (const auto& key : req["params"].getMemberNames()) {
        params[key] = req["params"][key];
      }
    }

    const bool hasBody = req["body"].isObject() && req.isMember("body");
    if (hasBody) {
      for (const auto& key : req["body"].getMemberNames()) {
        params[key] = req["body"][key];
      }
    }

    RouterCtx ctx = Json::objectValue;
    ctx["params"] = params;
    return ctx;
  }

  BoostHandler handler = [this](const BoostReq& req, BoostRes& res) {
    RouterCtx ctx = this->getCtx(req);

    this->router.request(ctx, [&](const std::optional<Route>& routeOpt) {
      const auto& contentTypeKey = boost::beast::http::field::content_type;
      Json::Value response = Json::objectValue;
      Json::StreamWriterBuilder writer;
      writer["indentation"] = "";

      if (!routeOpt) {
        response["code"] = 404;
        response["error"] = "Not found.";

        res.result(response["code"].asInt());
        res.set(contentTypeKey, "application/json");
        res.body() = Json::writeString(writer, response);
        return;
      }

      const Route rawRoute = *routeOpt;
      Json::Value route = Json::objectValue;
      route["method"] = *rawRoute.method;
      route["params"] = rawRoute.params;
      route["pattern"] = rawRoute.pattern;
      ctx["params"]["_state"]["route"] = route;
      response = rawRoute.controller(ctx);

      const bool isObject = response.isObject();
      if (!isObject) {
        res.result(200);
        res.set(contentTypeKey, "application/json");
        res.body() = Json::writeString(writer, response);
        return;
      }

      const bool hasCode = response.isMember("code");
      if (!hasCode) {
        res.result(200);
        res.set(contentTypeKey, "application/json");
        res.body() = Json::writeString(writer, response);
        return;
      }

      res.result(response["code"].asInt());
      res.set(contentTypeKey, "application/json");
      res.body() = Json::writeString(writer, response);
    });
  };

 public:
  Server() : server(env.SERVER_PORT) {
    routes(this->router);
    this->server.setHandler(this->handler);
  }

  void start() {
    this->server.start([]() {
      Logger::success("Server is running on port " + env.SERVER_PORT);
    });
  }

  void stop() {
    this->server.start([]() { Logger::success("Server stopped"); });
  }
};

int main(int argc, char* argv[]) {
  Server server;
  server.start();

  return 0;
}