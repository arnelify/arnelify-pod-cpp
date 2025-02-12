#ifndef ARNELIFY_BROKER_CPP
#define ARNELIFY_BROKER_CPP

#include <functional>
#include <future>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <vector>

#include "json.h"

#include "serializer/index.cpp"

#include "contracts/action.hpp"
#include "contracts/callback.hpp"
#include "contracts/ctx.hpp"

class ArnelifyBroker {
 private:
  std::map<const std::string, BrokerAction> actions;
  std::map<const std::string, BrokerRequest> req;
  std::map<const std::string, BrokerResponse> res;

  void consumer(const std::string& topic,
                std::function<void(const std::string&)> onMessage) {
    this->req[topic] = onMessage;
  };

  const Json::Value handler(const std::string& topic, Json::Value& ctx) {
    ctx["receivedAt"] = this->getDateTime();
    const BrokerAction& action = this->actions[topic];
    Json::Value res = Json::objectValue;
    res["content"] = action(ctx);
    res["createdAt"] = ctx["createdAt"];
    res["receivedAt"] = this->getDateTime();
    res["topic"] = ctx["topic"];
    res["uuid"] = ctx["uuid"];
    return res;
  }

  void producer(const std::string& topic, const std::string& message) {
    const BrokerRequest onMessage = this->req[topic];
    onMessage(message);
  };

  void receive(const Json::Value& res) {
    const std::string uuid = res["uuid"].asString();
    const BrokerResponse resolve = this->res[uuid];
    this->res.erase(uuid);
    resolve(res["content"]);
  }

  const Json::Value send(
      const std::string& topic, const Json::Value& params,
      const std::function<void(const std::string&)> producer) {
    std::promise<Json::Value> promise;
    std::future<Json::Value> future = promise.get_future();
    std::thread thread(
        [this, &topic, &params, &producer](std::promise<Json::Value>& promise) {
          const std::string uuid = this->getUuId();
          this->res[uuid] = [&promise](const Json::Value& res) {
            promise.set_value(res);
          };

          Ctx ctx = Json::objectValue;
          ctx["topic"] = topic;
          ctx["createdAt"] = this->getDateTime();
          ctx["receivedAt"] = Json::nullValue;
          ctx["params"] = params;
          ctx["uuid"] = uuid;

          const std::string message = Serializer::serialize(ctx);
          producer(message);
        },
        std::ref(promise));

    Json::Value response = future.get();
    thread.join();

    return response;
  }

 public:
  Json::Value call(const std::string& topic, const Json::Value& params) {
    return this->send(topic, params, [this, topic](const std::string& message) {
      this->producer(topic + ":req", message);
    });
  }

  const std::string getDateTime() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    const std::tm local_time = *std::localtime(&now_c);

    std::ostringstream oss;
    oss << std::put_time(&local_time, "%Y-%m-%d %H:%M:%S");
    return oss.str();
  }

  const std::string getUuId() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(10000, 19999);
    int random = dis(gen);
    const auto now = std::chrono::system_clock::now();
    const auto milliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch())
            .count();

    const std::string code = std::to_string(milliseconds) + std::to_string(random);
    std::hash<std::string> hasher;
    size_t v1 = hasher(code);
    size_t v2 = hasher(std::to_string(v1));
    unsigned char hash[16];
    for (int i = 0; i < 8; ++i) {
        hash[i] = (v1 >> (i * 8)) & 0xFF;
        hash[i + 8] = (v2 >> (i * 8)) & 0xFF;
    }

    std::stringstream ss;
    for (int i = 0; i < 16; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0')
           << static_cast<int>(hash[i]);
    }

    return ss.str();
  }

  void subscribe(const std::string& topic, const BrokerAction& action) {
    this->actions[topic] = action;

    this->consumer(topic + ":res", [this, topic](const std::string& message) {
      const Json::Value res = Serializer::deserialize(message);
      this->receive(res);
    });

    this->consumer(topic + ":req", [this](const std::string& message) {
      Ctx ctx = Serializer::deserialize(message);
      const std::string topic = ctx["topic"].asString();
      const Json::Value res = this->handler(topic, ctx);
      const std::string serialized = Serializer::serialize(res);
      this->producer(topic + ":res", serialized);
    });
  }
};

#endif