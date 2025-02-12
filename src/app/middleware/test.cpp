#ifndef TEST_MIDDLEWARE_CPP
#define TEST_MIDDLEWARE_CPP

#include "json.h"

#include "broker/src/cpp/contracts/ctx.hpp"
#include "logger/index.cpp"

Ctx testMiddleware(Ctx ctx) {

  Json::Value numbers = Json::arrayValue;
  numbers.append(3);
  numbers.append(3);
  numbers.append(3);
  
  ctx["params"]["numbers"] = numbers;
  Logger::primary("TestMiddleware: Let\'s start the test, guys.\n");
  return ctx;
}

#endif