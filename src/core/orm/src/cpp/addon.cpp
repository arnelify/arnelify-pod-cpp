#ifndef ARNELIFY_ORM_ADDON_CPP
#define ARNELIFY_ORM_ADDON_CPP

#include <future>
#include <iostream>
#include <thread>

#include "json.h"
#include "napi.h"

#include "index.cpp"

ArnelifyORM* orm = nullptr;

Napi::Value orm_create(const Napi::CallbackInfo& args) {
  Napi::Env env = args.Env();
  if (args.Length() < 1 || !args[0].IsString()) {
    Napi::TypeError::New(env,
                         "[Arnelify ORM]: C++ error: Expected optsWrapped.")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  Napi::String optsWrapped = args[0].As<Napi::String>();
  std::string serialized = optsWrapped.Utf8Value();

  Json::Value json;
  Json::CharReaderBuilder reader;
  std::string errors;

  std::istringstream iss(serialized);
  if (!Json::parseFromStream(reader, iss, &json, &errors)) {
    std::cout << "[Arnelify ORM]: C++ error: Invalid cOpts." << std::endl;
    exit(1);
  }

  const bool hasDriver =
      json.isMember("ORM_DRIVER") && json["ORM_DRIVER"].isString();
  if (!hasDriver) {
    std::cout << "[Arnelify ORM]: C++ error: 'ORM_DRIVER' is missing."
              << std::endl;
    exit(1);
  }

  const bool hasHost = json.isMember("ORM_HOST") && json["ORM_HOST"].isString();
  if (!hasHost) {
    std::cout << "[Arnelify ORM]: C++ error: 'ORM_HOST' is missing."
              << std::endl;
    exit(1);
  }

  const bool hasName = json.isMember("ORM_NAME") && json["ORM_NAME"].isString();
  if (!hasName) {
    std::cout << "[Arnelify ORM]: C++ error: 'ORM_NAME' is missing."
              << std::endl;
    exit(1);
  }

  const bool hasUser = json.isMember("ORM_USER") && json["ORM_USER"].isString();
  if (!hasUser) {
    std::cout << "[Arnelify ORM]: C++ error: 'ORM_USER' is missing."
              << std::endl;
    exit(1);
  }

  const bool hasPass = json.isMember("ORM_PASS") && json["ORM_PASS"].isString();
  if (!hasPass) {
    std::cout << "[Arnelify ORM]: C++ error: 'ORM_PASS' is missing."
              << std::endl;
    exit(1);
  }

  const bool hasPort = json.isMember("ORM_PORT") && json["ORM_PORT"].isInt();
  if (!hasPort) {
    std::cout << "[Arnelify ORM]: C++ error: 'ORM_PORT' is missing."
              << std::endl;
    exit(1);
  }

  ArnelifyORMOpts opts(json["ORM_DRIVER"].asString(),
                       json["ORM_HOST"].asString(), json["ORM_NAME"].asString(),
                       json["ORM_USER"].asString(), json["ORM_PASS"].asString(),
                       json["ORM_PORT"].asInt());

  orm = new ArnelifyORM(opts);

  return env.Undefined();
}

Napi::Value orm_destroy(const Napi::CallbackInfo& args) {
  Napi::Env env = args.Env();

  delete orm;
  orm = nullptr;
  return env.Undefined();
}

Napi::Value orm_exec(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (!info.Length() || !info[0].IsString()) {
    Napi::TypeError::New(env, "[Arnelify ORM]: C++ error: query is missing.")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  if (2 > info.Length() || !info[1].IsString()) {
    Napi::TypeError::New(env, "[Arnelify ORM]: C++ error: bindings is missing.")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  const std::string query = info[0].As<Napi::String>();
  const std::string serialized = info[1].As<Napi::String>();
  std::vector<std::string> bindings;

  Json::Value deserialized;
  Json::CharReaderBuilder reader;
  std::string errors;

  std::istringstream iss(serialized);
  if (!Json::parseFromStream(reader, iss, &deserialized, &errors)) {
    std::cout << "[Arnelify ORM]: C++ error: bindings must be a valid JSON."
              << std::endl;
    exit(1);
  }

  for (int i = 0; deserialized.size() > i; ++i) {
    const Json::Value& value = deserialized[i];
    bindings.emplace_back(value.asString());
  }

  ArnelifyORMRes res = orm->exec(query, bindings);
  Json::Value json = orm->toJson(res);

  Json::StreamWriterBuilder writer;
  writer["indentation"] = "";
  writer["emitUTF8"] = true;

  const std::string out = Json::writeString(writer, json);
  return Napi::String::New(env, out);
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set("orm_create", Napi::Function::New(env, orm_create));
  exports.Set("orm_destroy", Napi::Function::New(env, orm_destroy));
  exports.Set("orm_exec", Napi::Function::New(env, orm_exec));
  return exports;
}

NODE_API_MODULE(ARNELIFY_ORM, Init)

#endif