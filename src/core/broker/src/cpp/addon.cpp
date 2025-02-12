#ifndef ARNELIFY_BROKER_ADDON_CPP
#define ARNELIFY_BROKER_ADDON_CPP

#include <future>
#include <iostream>
#include <thread>

#include "json.h"
#include "napi.h"

#include "index.cpp"

ArnelifyBroker* broker = nullptr;

Napi::Value broker_create(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  broker = new ArnelifyBroker();
  return env.Undefined();
}

Napi::Value broker_destroy(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  broker = nullptr;
  return env.Undefined();
}

Napi::Value broker_get_datetime(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  return Napi::String::New(env, broker->getDateTime());
}

Napi::Value broker_get_uuid(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  return Napi::String::New(env, broker->getUuId());
}

Napi::Value broker_serialize(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (!info.Length() || !info[0].IsString()) {
    Napi::TypeError::New(env,
                         "[Arnelify Broker]: C++ error: "
                         "deserialized is missing.")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  std::string deserialized = info[0].As<Napi::String>();

  Json::Value ctx;
  Json::CharReaderBuilder reader;
  std::string errors;
  std::istringstream iss(deserialized);
  if (!Json::parseFromStream(reader, iss, &ctx, &errors)) {
    Napi::TypeError::New(
        env, "[Arnelify Broker]: C++ error: Serialized must be a valid JSON.")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  return Napi::String::New(env, Serializer::serialize(ctx));
}

Napi::Value broker_deserialize(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  if (!info.Length() || !info[0].IsString()) {
    Napi::TypeError::New(env,
                         "[Arnelify Broker]: C++ error: "
                         "deserialized is missing.")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  std::string serialized = info[0].As<Napi::String>();
  const Json::Value ctx = Serializer::deserialize(serialized);
  Json::StreamWriterBuilder writer;
  writer["indentation"] = "";
  writer["emitUTF8"] = true;

  return Napi::String::New(env, Json::writeString(writer, ctx));
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set("broker_create", Napi::Function::New(env, broker_create));
  exports.Set("broker_destroy", Napi::Function::New(env, broker_destroy));
  exports.Set("broker_get_datetime",
              Napi::Function::New(env, broker_get_datetime));
  exports.Set("broker_get_uuid", Napi::Function::New(env, broker_get_uuid));
  exports.Set("broker_serialize", Napi::Function::New(env, broker_serialize));
  exports.Set("broker_deserialize",
              Napi::Function::New(env, broker_deserialize));
  return exports;
}

NODE_API_MODULE(ARNELIFY_BROKER, Init)

#endif