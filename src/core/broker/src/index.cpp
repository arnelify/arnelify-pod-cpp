#ifndef ARNELIFY_BROKER_CPP
#define ARNELIFY_BROKER_CPP

#include <dlfcn.h>
#include <filesystem>
#include <functional>
#include <future>
#include <iostream>
#include <stdexcept>

#include "json.h"

#include "cpp/contracts/action.hpp"
#include "cpp/contracts/callback.hpp"
#include "cpp/contracts/ctx.hpp"

class ArnelifyBroker {
 private:
  void* lib = nullptr;
  std::filesystem::path libPath;
  std::map<const std::string, BrokerAction> actions;
  std::map<const std::string, BrokerRequest> req;
  std::map<const std::string, BrokerResponse> res;

  const char* (*broker_get_datetime)();
  const char* (*broker_get_uuid)();
  const char* (*broker_serialize)(const char*);
  const char* (*broker_deserialize)(const char*);
  void (*broker_free)(const char*);

  template <typename T>
  void loadFunction(const std::string& name, T& func) {
    func = reinterpret_cast<T>(dlsym(this->lib, name.c_str()));
    if (!func) {
      throw std::runtime_error(dlerror());
    }
  }

  const std::string getDateTime() {
    const char* cDatetime = this->broker_get_datetime();
    const std::string datetime = cDatetime;
    this->broker_free(cDatetime);
    return datetime;
  }

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
          const std::string uuid = this->getUuid();
          this->res[uuid] = [&promise](const Json::Value& res) {
            promise.set_value(res);
          };

          Ctx ctx = Json::objectValue;
          ctx["topic"] = topic;
          ctx["createdAt"] = this->getDateTime();
          ctx["receivedAt"] = Json::nullValue;
          ctx["params"] = params;
          ctx["uuid"] = uuid;

          const std::string message = this->serialize(ctx);
          producer(message);
        },
        std::ref(promise));

    Json::Value response = future.get();
    thread.join();

    return response;
  }

  const std::string getLibPath() {
    const std::filesystem::path scriptDir =
        std::filesystem::absolute(__FILE__).parent_path();

    const bool hasExitSegment = scriptDir.string().ends_with("..");
    if (!hasExitSegment) {
      std::filesystem::path libDir = scriptDir.parent_path();
      const std::string libPath = libDir / "build" / "index.so";
      return libPath;
    }

    std::istringstream stream(scriptDir);
    std::deque<std::string> segments;
    std::string segment;

    while (std::getline(stream, segment, '/')) {
      if (segment.empty()) continue;

      if (segment == "..") {
        if (!segments.empty()) {
          segments.pop_back();
          segments.pop_back();
        }
        continue;
      }

      segments.push_back(segment);
    }

    std::string libPath;
    for (const auto& segment : segments) {
      libPath += "/" + segment;
    }

    libPath += "/build/index.so";
    return libPath;
  }

  const std::string getUuid() {
    const char* cUuId = this->broker_get_uuid();
    const std::string uuid = cUuId;
    this->broker_free(cUuId);
    return uuid;
  }

 public:
  ArnelifyBroker() {
    const std::string libPath = this->getLibPath();
    this->lib = dlopen(libPath.c_str(), RTLD_LAZY);
    if (!this->lib) throw std::runtime_error(dlerror());

    loadFunction("broker_get_datetime", this->broker_get_datetime);
    loadFunction("broker_get_uuid", this->broker_get_uuid);
    loadFunction("broker_serialize", this->broker_serialize);
    loadFunction("broker_deserialize", this->broker_deserialize);
    loadFunction("broker_free", this->broker_free);
  }

  Json::Value call(const std::string& topic, const Json::Value& params) {
    return this->send(topic, params, [this, topic](const std::string& message) {
      this->producer(topic + ":req", message);
    });
  }

  void consumer(const std::string& topic,
                std::function<void(const std::string&)> onMessage) {
    this->req[topic] = onMessage;
  };

  const Json::Value deserialize(const std::string& serialized) {
    const char* cDeserialized = this->broker_deserialize(serialized.c_str());
    const std::string deserialized = cDeserialized;
    this->broker_free(cDeserialized);

    Json::Value ctx;
    Json::CharReaderBuilder reader;
    std::string errors;
    std::istringstream iss(deserialized);
    if (!Json::parseFromStream(reader, iss, &ctx, &errors)) {
      std::cout << "[ArnelifyBroker FFI]: C++ error: cDeserialized must be a "
                   "valid JSON."
                << std::endl;
      exit(1);
    }

    return ctx;
  }

  void producer(const std::string& topic, const std::string& message) {
    const BrokerRequest onMessage = this->req[topic];
    onMessage(message);
  };

  const std::string serialize(Json::Value& ctx) {
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    writer["emitUTF8"] = true;

    const std::string deserialized = Json::writeString(writer, ctx);
    const char* cSerialized = this->broker_serialize(deserialized.c_str());
    const std::string serialized = cSerialized;
    this->broker_free(cSerialized);
    return serialized;
  }

  void subscribe(const std::string& topic, const BrokerAction& action) {
    this->actions[topic] = action;

    this->consumer(topic + ":res", [this, topic](const std::string& message) {
      const Json::Value res = this->deserialize(message);
      this->receive(res);
    });

    this->consumer(topic + ":req", [this](const std::string& message) {
      Ctx ctx = this->deserialize(message);
      const std::string topic = ctx["topic"].asString();

      Json::Value res = this->handler(topic, ctx);
      const std::string serialized = this->serialize(res);
      this->producer(topic + ":res", serialized);
    });
  }
};

#endif