#ifndef TEST_MIDDLEWARE_HPP
#define TEST_MIDDLEWARE_HPP

#include "json.h"

#include "broker/contracts/ctx.h"

#include "logger/index.hpp"

BrokerCtx testMiddleware(BrokerCtx ctx) {

  Json::Value numbers = Json::arrayValue;
  numbers.append(3);
  numbers.append(3);
  numbers.append(3);
  
  ctx["params"]["numbers"] = numbers;
  Logger::primary("TestMiddleware: Let\'s start the test, guys.");
  return ctx;
}

#endif