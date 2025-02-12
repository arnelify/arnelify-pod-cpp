#ifndef FIRST_SERVICE_CPP
#define FIRST_SERVICE_CPP

#include <iostream>

#include "broker/index.cpp"
#include "logger/index.cpp"

#include "json.h"

class First {
 private:
  const std::string join(const Json::Value& numbers,
                         const std::string& delimeter) {
    std::ostringstream oss;
    for (Json::Value::ArrayIndex i = 0; i < numbers.size(); ++i) {
      oss << numbers[i].asInt();
      if (i < numbers.size() - 1) {
        oss << delimeter;
      }
    }
    return oss.str();
  }

 public:
  const Json::Value welcome(const Ctx& ctx) {
    const Json::Value& params = ctx["params"];
    const Json::Value& numbers = params["numbers"];

    const std::string joined = this->join(numbers, " + ");

    Logger::primary("First: Hey, Second! Can you tell me what " + joined +
                    " equals?\n");

    const Json::Value secondResponse =
        broker->call("second.welcome", ctx["params"]);
    if (secondResponse["code"].asInt() != 200) return secondResponse;

    const Json::Value response = secondResponse["success"]["response"];
    Logger::primary("First: Great, Second! Thanks a lot!\n");
    return response;
  }
};

#endif