#ifndef ARNELIFY_SERVER_ADDON_CPP
#define ARNELIFY_SERVER_ADDON_CPP

#include <future>
#include <iostream>
#include <thread>

#include "json.h"
#include "napi.h"

#include "uds/index.cpp"
#include "index.cpp"

ArnelifyServer* server = nullptr;
ArnelifyUnixDomainSocketClient* uds = nullptr;

Napi::Value server_create(const Napi::CallbackInfo& args) {
  Napi::Env env = args.Env();
  if (args.Length() < 1 || !args[0].IsString()) {
    Napi::TypeError::New(env,
                         "[Arnelify Server]: C++ error: Expected optsWrapped.")
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
    Napi::TypeError::New(env,
                         "[ArnelifyServer]: C++ error: Invalid optsWrapper.")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  const bool hasAllowEmptyFiles = json.isMember("SERVER_ALLOW_EMPTY_FILES") &&
                                  json["SERVER_ALLOW_EMPTY_FILES"].isBool();
  if (!hasAllowEmptyFiles) {
    Napi::TypeError::New(env,
                         "[Arnelify Server]: C++ error: "
                         "'SERVER_ALLOW_EMPTY_FILES' is missing.")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  const bool hasBlockSizeKb = json.isMember("SERVER_BLOCK_SIZE_KB") &&
                              json["SERVER_BLOCK_SIZE_KB"].isInt();
  if (!hasBlockSizeKb) {
    Napi::TypeError::New(env,
                         "[Arnelify Server]: C++ error: "
                         "'SERVER_BLOCK_SIZE_KB' is missing.")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  const bool hasCharset =
      json.isMember("SERVER_CHARSET") && json["SERVER_CHARSET"].isString();
  if (!hasCharset) {
    Napi::TypeError::New(env,
                         "[Arnelify Server]: C++ error: "
                         "'SERVER_CHARSET' is missing.")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  const bool hasGzip =
      json.isMember("SERVER_GZIP") && json["SERVER_GZIP"].isBool();
  if (!hasGzip) {
    Napi::TypeError::New(env,
                         "[Arnelify Server]: C++ error: "
                         "'SERVER_GZIP' is missing.")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  const bool hasKeepExtensions = json.isMember("SERVER_KEEP_EXTENSIONS") &&
                                 json["SERVER_KEEP_EXTENSIONS"].isBool();
  if (!hasKeepExtensions) {
    Napi::TypeError::New(env,
                         "[Arnelify Server]: C++ error: "
                         "'SERVER_KEEP_EXTENSIONS' is missing.")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  const bool hasMaxFields =
      json.isMember("SERVER_MAX_FIELDS") && json["SERVER_MAX_FIELDS"].isInt();
  if (!hasMaxFields) {
    Napi::TypeError::New(env,
                         "[Arnelify Server]: C++ error: "
                         "'SERVER_MAX_FIELDS' is missing.")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  const bool hasMaxFieldsSizeTotalMb =
      json.isMember("SERVER_MAX_FIELDS_SIZE_TOTAL_MB") &&
      json["SERVER_MAX_FIELDS_SIZE_TOTAL_MB"].isInt();
  if (!hasMaxFieldsSizeTotalMb) {
    Napi::TypeError::New(env,
                         "[Arnelify Server]: C++ error: "
                         "'SERVER_MAX_FIELDS_SIZE_TOTAL_MB' is missing.")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  const bool hasMaxFiles =
      json.isMember("SERVER_MAX_FILES") && json["SERVER_MAX_FILES"].isInt();
  if (!hasMaxFiles) {
    Napi::TypeError::New(env,
                         "[Arnelify Server]: C++ error: "
                         "'SERVER_MAX_FILES' is missing.")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  const bool hasMaxFilesSizeTotalMb =
      json.isMember("SERVER_MAX_FILES_SIZE_TOTAL_MB") &&
      json["SERVER_MAX_FILES_SIZE_TOTAL_MB"].isInt();
  if (!hasMaxFilesSizeTotalMb) {
    Napi::TypeError::New(env,
                         "[Arnelify Server]: C++ error: "
                         "'SERVER_MAX_FILES_SIZE_TOTAL_MB' is missing.")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  const bool hasMaxFileSizeMb = json.isMember("SERVER_MAX_FILE_SIZE_MB") &&
                                json["SERVER_MAX_FILE_SIZE_MB"].isInt();
  if (!hasMaxFileSizeMb) {
    Napi::TypeError::New(env,
                         "[Arnelify Server]: C++ error: "
                         "'SERVER_MAX_FILE_SIZE_MB' is missing.")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  const bool hasPort =
      json.isMember("SERVER_PORT") && json["SERVER_PORT"].isInt();
  if (!hasPort) {
    Napi::TypeError::New(env,
                         "[Arnelify Server]: C++ error: "
                         "'SERVER_PORT' is missing.")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  const bool hasQueueLimit =
      json.isMember("SERVER_QUEUE_LIMIT") && json["SERVER_QUEUE_LIMIT"].isInt();
  if (!hasQueueLimit) {
    Napi::TypeError::New(env,
                         "[Arnelify Server]: C++ error: "
                         "'SERVER_QUEUE_LIMIT' is missing.")
        .ThrowAsJavaScriptException();
    return env.Undefined();
  }

  const bool hasUploadDir = json.isMember("SERVER_UPLOAD_DIR") &&
                            json["SERVER_UPLOAD_DIR"].isString();
  if (!hasUploadDir) json["SERVER_UPLOAD_DIR"] = "./src/storage/upload";

  const bool hasSocketPath = json.isMember("SERVER_SOCKET_PATH") &&
                             json["SERVER_SOCKET_PATH"].isString();
  if (!hasSocketPath) json["SERVER_SOCKET_PATH"] = "/tmp/arnelify.sock";

  ArnelifyUnixDomainSocketClientOpts udsOpts(
      json["SERVER_BLOCK_SIZE_KB"].asInt(),
      json["SERVER_SOCKET_PATH"].asString());
  uds = new ArnelifyUnixDomainSocketClient(udsOpts);

  ArnelifyServerOpts opts(
      json["SERVER_ALLOW_EMPTY_FILES"].asBool(),
      json["SERVER_BLOCK_SIZE_KB"].asInt(), json["SERVER_CHARSET"].asString(),
      json["SERVER_GZIP"].asBool(), json["SERVER_KEEP_EXTENSIONS"].asBool(),
      json["SERVER_MAX_FIELDS"].asInt(),
      json["SERVER_MAX_FIELDS_SIZE_TOTAL_MB"].asInt(),
      json["SERVER_MAX_FILES"].asInt(),
      json["SERVER_MAX_FILES_SIZE_TOTAL_MB"].asInt(),
      json["SERVER_MAX_FILE_SIZE_MB"].asInt(), json["SERVER_PORT"].asInt(),
      json["SERVER_QUEUE_LIMIT"].asInt(), json["SERVER_UPLOAD_DIR"].asString());

  server = new ArnelifyServer(opts);
  server->setHandler([](const Req& req, Res res) {
    std::promise<const std::string> promise;
    std::future<const std::string> future = promise.get_future();
    std::thread thread(
        [&req](std::promise<const std::string>& promise) {
          const std::string uuid = uds->createUuId();
          uds->on(uuid, [&promise](const std::string& message) {
            promise.set_value(message);
          });

          Json::StreamWriterBuilder writer;
          writer["indentation"] = "";
          writer["emitUTF8"] = true;

          Json::Value json;
          json["uuid"] = uuid;
          json["content"] = req;

          uds->write(Json::writeString(writer, json));
        },
        std::ref(promise));

    const std::string serialized = future.get();
    thread.join();

    Json::Value json;
    Json::CharReaderBuilder reader;
    std::string errors;

    std::istringstream iss(serialized);
    if (!Json::parseFromStream(reader, iss, &json, &errors)) {
      Json::StreamWriterBuilder writer;
      writer["indentation"] = "";
      writer["emitUTF8"] = true;

      Json::Value _stdout;
      _stdout["isError"] = true;
      _stdout["message"] = "C error: cRes must be a valid JSON.";

      Json::Value content;
      content["_stdout"] = _stdout;

      Json::Value json;
      json["uuid"] = uds->createUuId();
      json["content"] = content;

      uds->write(Json::writeString(writer, json));

      res->addBody("");
      res->end();
      return;
    }

    const bool hasCode = json.isMember("code");
    if (hasCode) {
      res->setCode(json["code"].asInt());
    }

    const bool hasHeaders =
        json.isMember("headers") && json["headers"].isObject();
    if (hasHeaders) {
      for (const Json::String& header : json["headers"].getMemberNames()) {
        res->setHeader(header, json["headers"][header].asString());
      }
    }

    if (json.isMember("filePath") && json.isMember("isStatic") &&
        json["filePath"].isString() && json["isStatic"].isBool() &&
        !json["filePath"].asString().empty()) {
      res->setFile(json["filePath"].asString(), json["isStatic"].asBool());
      res->end();
      return;
    }

    if (json.isMember("body") && json["body"].isString()) {
      res->addBody(json["body"].asString());
      res->end();
      return;
    }

    res->addBody("");
    res->end();
  });

  return env.Undefined();
}

Napi::Value server_destroy(const Napi::CallbackInfo& args) {
  Napi::Env env = args.Env();

  delete server;
  server = nullptr;

  return env.Undefined();
}

Napi::Value server_start(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();
  bool error = false;

  uds->connect([&error](const std::string& message, const bool& isError) {
    if (isError) {
      std::cout << "\033[31m"
                << "[Arnelify Unix Domain Socket]: C++ Error: " << message
                << "\033[0m" << std::endl;
      error = isError;
    }
  });

  if (error) return env.Undefined();

  std::thread thread([]() {
    server->start([](const std::string& message, const bool& isError) {
      Json::StreamWriterBuilder writer;
      writer["indentation"] = "";
      writer["emitUTF8"] = true;

      Json::Value _stdout;
      _stdout["isError"] = isError;
      _stdout["message"] = message;

      Json::Value content;
      content["_stdout"] = _stdout;

      Json::Value json;
      json["uuid"] = uds->createUuId();
      json["content"] = content;

      uds->write(Json::writeString(writer, json));
    });
  });

  thread.detach();
  return env.Undefined();
}

Napi::Value server_stop(const Napi::CallbackInfo& info) {
  Napi::Env env = info.Env();

  server->stop();
  return env.Undefined();
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
  exports.Set("server_create", Napi::Function::New(env, server_create));
  exports.Set("server_destroy", Napi::Function::New(env, server_destroy));
  exports.Set("server_start", Napi::Function::New(env, server_start));
  exports.Set("server_stop", Napi::Function::New(env, server_stop));
  return exports;
}

NODE_API_MODULE(ARNELIFY_SERVER, Init)

#endif