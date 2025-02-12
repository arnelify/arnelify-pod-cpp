#ifndef ARNELIFY_BROKER_TEST_CPP
#define ARNELIFY_BROKER_TEST_CPP

#include <iostream>

#include "json.h"

#include "../index.cpp"

int main(int argc, char* argv[]) {

  ArnelifyBroker* broker = new ArnelifyBroker();

  broker->subscribe("second.welcome", [](const Ctx& ctx) {
    Json::Value res;
    res["params"] = ctx["params"];
    res["params"]["success"] = "Welcome to Arnelify Broker";
    return res;
  });

  broker->subscribe("first.welcome", [broker](const Ctx& ctx) {
    Json::Value res;
    res["params"] = ctx["params"];
    res["params"]["code"] = 200;
    return broker->call("second.welcome", res["params"]);
  });

  Json::Value params;
  params["code"] = 0;
  params["success"] = "";

  Json::Value ctx;
  ctx["params"] = params;

  const Json::Value res = broker->call("first.welcome", ctx["params"]);

  Json::StreamWriterBuilder writer;
  writer["indentation"] = "";
  writer["emitUTF8"] = true;

  std::cout << "[Arnelify Broker]: Response: " 
    << Json::writeString(writer, res["params"]) << std::endl;
  return 0;
}

#endif