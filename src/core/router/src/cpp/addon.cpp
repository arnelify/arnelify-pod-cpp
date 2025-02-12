#ifndef ARNELIFY_ROUTER_ADDON_CPP
#define ARNELIFY_ROUTER_ADDON_CPP

#include <future>
#include <iostream>
#include <thread>

#include "json.h"
#include "napi.h"

#include "index.cpp"

ArnelifyRouter* router = nullptr;

Napi::Value router_create(const Napi::CallbackInfo& args) {
  Napi::Env env = args.Env();

  router = new ArnelifyRouter();
  return env.Undefined();
}

Napi::Value router_destroy(const Napi::CallbackInfo& args) {
  Napi::Env env = args.Env();

  delete router;
  router = nullptr;
  return env.Undefined();
}

Napi::Value router_any(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (!info.Length() || !info[0].IsString()) {
    Napi::TypeError::New(env,
                         "[Arnelify Router]: C++ error: "
                         "pattern is missing.")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  std::string pattern = info[0].As<Napi::String>();
  router->any(pattern, [](const Ctx& ctx) { return Json::nullValue; });
  return env.Undefined();
}

Napi::Value router_get(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (!info.Length() || !info[0].IsString()) {
    Napi::TypeError::New(env,
                         "[Arnelify Router]: C++ error: "
                         "pattern is missing.")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  std::string pattern = info[0].As<Napi::String>();
  router->get(pattern, [](const Ctx& ctx) { return Json::nullValue; });
  return env.Undefined();
}

Napi::Value router_post(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (!info.Length() || !info[0].IsString()) {
    Napi::TypeError::New(env,
                         "[Arnelify Router]: C++ error: "
                         "pattern is missing.")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  std::string pattern = info[0].As<Napi::String>();
  router->post(pattern, [](const Ctx& ctx) { return Json::nullValue; });
  return env.Undefined();
}

Napi::Value router_put(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (!info.Length() || !info[0].IsString()) {
    Napi::TypeError::New(env,
                         "[Arnelify Router]: C++ error: "
                         "pattern is missing.")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  std::string pattern = info[0].As<Napi::String>();
  router->put(pattern, [](const Ctx& ctx) { return Json::nullValue; });
  return env.Undefined();
}

Napi::Value router_patch(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (!info.Length() || !info[0].IsString()) {
    Napi::TypeError::New(env,
                         "[Arnelify Router]: C++ error: "
                         "pattern is missing.")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  std::string pattern = info[0].As<Napi::String>();
  router->patch(pattern, [](const Ctx& ctx) { return Json::nullValue; });
  return env.Undefined();
}

Napi::Value router_delete(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (!info.Length() || !info[0].IsString()) {
    Napi::TypeError::New(env,
                         "[Arnelify Router]: C++ error: "
                         "pattern is missing.")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  std::string pattern = info[0].As<Napi::String>();
  router->delete_(pattern, [](const Ctx& ctx) { return Json::nullValue; });
  return env.Undefined();
}

Napi::Value router_find(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  if (!info.Length() || !info[0].IsString()) {
    Napi::TypeError::New(env,
                         "[Arnelify Router]: C++ error: "
                         "method is missing.")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  if (2 > info.Length() || !info[1].IsString()) {
    Napi::TypeError::New(env,
                         "[Arnelify Router]: C++ error: "
                         "path is missing.")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  std::string method = info[0].As<Napi::String>();
  std::string path = info[1].As<Napi::String>();

  const std::optional<Route> routeOpt = router->find(method, path);
  if (!routeOpt) return Napi::String::New(env, "{}");

  const Route route = *routeOpt;
  Json::Value deserialized;
  deserialized["id"] = route.id;
  deserialized["pattern"] = route.pattern;
  deserialized["params"] = route.params;
  if (route.method) {
    deserialized["method"] = *route.method;
  } else {
    deserialized["method"] = Json::nullValue;
  }

  Json::StreamWriterBuilder writer;
  writer["indentation"] = "";
  writer["emitUTF8"] = true;

  std::string serialized = Json::writeString(writer, deserialized);
  return Napi::String::New(env, serialized);
}

Napi::Value router_reset(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  router->reset();
  return env.Undefined();
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set("router_create", Napi::Function::New(env, router_create));
  exports.Set("router_destroy", Napi::Function::New(env, router_destroy));
  exports.Set("router_any", Napi::Function::New(env, router_any));
  exports.Set("router_get", Napi::Function::New(env, router_get));
  exports.Set("router_post", Napi::Function::New(env, router_post));
  exports.Set("router_put", Napi::Function::New(env, router_put));
  exports.Set("router_patch", Napi::Function::New(env, router_patch));
  exports.Set("router_delete", Napi::Function::New(env, router_delete));
  exports.Set("router_find", Napi::Function::New(env, router_find));
  exports.Set("router_reset", Napi::Function::New(env, router_reset));
  return exports;
}

NODE_API_MODULE(ARNELIFY_ROUTER, Init)

#endif