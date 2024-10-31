#ifndef SECOND_SERVICE_HPP
#define SECOND_SERVICE_HPP

#include <iostream>

#include "json.h"

#include "broker/contracts/ctx.h"

#include "broker/index.hpp"
#include "logger/index.hpp"

class Second {
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

  const std::string stringify(Json::Value& json) {
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    
    return Json::writeString(writer, json);
  }

 public:
  Json::Value welcome(const BrokerCtx& ctx) {
    const Json::Value& params = ctx["params"];
    const Json::Value& numbers = params["numbers"];

    int result = 0;
    for (const auto& number : numbers) {
      result += number.asInt();
    }

    Json::Value res = Json::objectValue;
    res["code"] = 200;
    res["success"] = "Welcome to Arnelify POD framework.";

    const std::string joined = this->join(numbers, " + ");
    Logger::primary("Second: Hi, First! The result of " + joined +
                    " is = " + std::to_string(result) + ".");

    std::string resString = this->stringify(res);
    Logger::primary("Second: Here's your response: " + resString);

    Json::Value success = Json::objectValue;
    success["result"] = result;
    success["response"] = res;

    Json::Value response = Json::objectValue;
    response["code"] = 200;
    response["success"] = success;
    return response;
  }
};

#endif