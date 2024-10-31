#ifndef BROKER_HPP
#define BROKER_HPP

#include <random>
#include <future>
#include <iomanip>
#include <sstream>
#include <functional>

#include <functional>
#include <iostream>
#include <vector>

#include "boost/functional/hash.hpp"
#include "json.h"

#include "contracts/action.h"
#include "contracts/callback.h"
#include "contracts/ctx.h"

#include "serializer/index.hpp"
#include "logger/index.hpp"

class Broker {
 private:
  bool isDefaultConsumer;
  std::map<const std::string, BrokerAction> actions;
  std::map<const std::string, BrokerRequest> req;
  std::map<const std::string, BrokerResponse> res;
  std::function<void(const std::string&,
                     std::function<void(const std::string&)>)>
      consumer;
  std::function<void(const std::string&, const std::string&)> producer;

  const std::string getDateTime() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    const std::tm local_time = *std::localtime(&now_c);
    std::ostringstream oss;
    oss << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");
    return oss.str();
  }

  const std::string getRequestId() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(10000, 19999);
    int random = dis(gen);

    const auto now = std::chrono::system_clock::now();
    const auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch())
            .count();
    const std::string code =
        std::to_string(milliseconds) + std::to_string(random);

    unsigned char hash[20];
    boost::hash<std::string> hasher;
    size_t hash_value = hasher(code);

    for (int i = 0; i < 20; ++i) {
      hash[i] = (hash_value >> (i * 8)) & 0xFF;
    }

    std::stringstream ss;
    for (int i = 0; i < 20; ++i) {
      ss << std::hex << std::setw(2) << std::setfill('0')
         << static_cast<int>(hash[i]);
    }

    return ss.str();
  }

  void requestHandler(const std::string& topic) {
    std::function<void(const std::string&)> onRequest =
        [this](const std::string& message) {
          BrokerCtx ctx = Serializer::deserialize(message);
          const std::string topic = ctx["topic"].asString();
          const std::string requestId = ctx["requestId"].asString();
          ctx["receivedAt"] = this->getDateTime();

          const BrokerAction& action = this->actions[topic];
          const Json::Value response = action(ctx);

          Json::Value res = Json::objectValue;
          res["content"] = response;
          res["createdAt"] = ctx["createdAt"];
          res["receivedAt"] = this->getDateTime();
          res["requestId"] = ctx["requestId"];
          res["topic"] = ctx["topic"];

          this->sendResponse(topic, res);
        };

    this->consumer(topic + "-req", onRequest);
  };

  void responseHandler(const std::string& topic) {
    std::function<void(const std::string&)> onResponse =
        [this, topic](const std::string& message) {
          const Json::Value response = Serializer::deserialize(message);
          const std::string requestId = response["requestId"].asString();
          const BrokerResponse resolve = this->res[requestId];
          this->res.erase(requestId);

          resolve(response["content"]);
        };

    this->consumer(topic + "-res", onResponse);
  };

  void sendResponse(const std::string& topic, const Json::Value& response) {
    const std::string serialized = Serializer::serialize(response);
    this->producer(topic + "-res", serialized);
  }

  Json::Value getResponse(const std::string& topic, const Json::Value& params) {
    std::promise<Json::Value> promise;
    std::future<Json::Value> future = promise.get_future();
    std::thread thread(
        [this, &topic, &params](std::promise<Json::Value>& promise) {
          const std::string requestId = this->getRequestId();

          BrokerCtx ctx = Json::objectValue;
          ctx["topic"] = topic;
          ctx["requestId"] = requestId;
          ctx["createdAt"] = this->getDateTime();
          ctx["receivedAt"] = Json::nullValue;
          ctx["params"] = params;

          const std::string message = Serializer::serialize(ctx);
          this->res[requestId] = [&promise](const Json::Value& response) {
            promise.set_value(response);
          };

          this->producer(topic + "-req", message);
        },
        std::ref(promise));

    Json::Value response = future.get();
    thread.join();

    return response;
  }

 public:
  Broker() : isDefaultConsumer(false) {
    this->setConsumer(
        [this](const std::string& topic,
               std::function<void(const std::string&)> onMessage) {
          this->defaultConsumer(topic, onMessage);
        });

    this->setProducer(
        [this](const std::string& topic, const std::string& message) {
          this->defaultProducer(topic, message);
        });
  }

  void defaultConsumer(const std::string topic, BrokerRequest onMessage) {
    this->isDefaultConsumer = true;
    this->req[topic] = onMessage;
  }

  void defaultProducer(const std::string& topic, const std::string& message) {
    const BrokerRequest consumer = this->req[topic];
    consumer(message);
  }

  void setConsumer(std::function<void(const std::string&,
                                      std::function<void(const std::string&)>)>
                       consumer) {
    this->consumer = consumer;
  }

  void setProducer(
      std::function<void(const std::string&, const std::string&)> producer) {
    this->producer = producer;
  }

  void subscribe(const std::string& topic, const BrokerAction& action) {
    this->actions[topic] = action;
    this->requestHandler(topic);
    this->responseHandler(topic);

    const bool& isDefault = this->isDefaultConsumer;
    if (isDefault) Logger::warning("topic registered '" + topic + "'.");
  }

  Json::Value call(const std::string& topic, const Json::Value& params) {
    return this->getResponse(topic, params);
  }
};

Broker broker;

#endif