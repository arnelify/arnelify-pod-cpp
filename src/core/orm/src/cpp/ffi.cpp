#ifndef ARNELIFY_ORM_FFI_CPP
#define ARNELIFY_ORM_FFI_CPP

#include "index.cpp"

extern "C" {

ArnelifyORM* orm = nullptr;

void orm_create(const char* cOpts) {
  Json::Value json;
  Json::CharReaderBuilder reader;
  std::string errors;

  std::istringstream iss(cOpts);
  if (!Json::parseFromStream(reader, iss, &json, &errors)) {
    std::cout << "[ArnelifyORM FFI]: C error: Invalid cOpts." << std::endl;
    exit(1);
  }

  const bool hasDriver =
      json.isMember("ORM_DRIVER") && json["ORM_DRIVER"].isString();
  if (!hasDriver) {
    std::cout << "[ArnelifyORM FFI]: C error: 'ORM_DRIVER' is missing."
              << std::endl;
    exit(1);
  }

  const bool hasHost = json.isMember("ORM_HOST") && json["ORM_HOST"].isString();
  if (!hasHost) {
    std::cout << "[ArnelifyORM FFI]: C error: 'ORM_HOST' is missing."
              << std::endl;
    exit(1);
  }

  const bool hasName = json.isMember("ORM_NAME") && json["ORM_NAME"].isString();
  if (!hasName) {
    std::cout << "[ArnelifyORM FFI]: C error: 'ORM_NAME' is missing."
              << std::endl;
    exit(1);
  }

  const bool hasUser = json.isMember("ORM_USER") && json["ORM_USER"].isString();
  if (!hasUser) {
    std::cout << "[ArnelifyORM FFI]: C error: 'ORM_USER' is missing."
              << std::endl;
    exit(1);
  }

  const bool hasPass = json.isMember("ORM_PASS") && json["ORM_PASS"].isString();
  if (!hasPass) {
    std::cout << "[ArnelifyORM FFI]: C error: 'ORM_PASS' is missing."
              << std::endl;
    exit(1);
  }

  const bool hasPort = json.isMember("ORM_PORT") && json["ORM_PORT"].isInt();
  if (!hasPort) {
    std::cout << "[ArnelifyORM FFI]: C error: 'ORM_PORT' is missing."
              << std::endl;
    exit(1);
  }

  ArnelifyORMOpts opts(json["ORM_DRIVER"].asString(),
                       json["ORM_HOST"].asString(), json["ORM_NAME"].asString(),
                       json["ORM_USER"].asString(), json["ORM_PASS"].asString(),
                       json["ORM_PORT"].asInt());

  orm = new ArnelifyORM(opts);
}

void orm_destroy() {
  delete orm;
  orm = nullptr;
}

const char* orm_exec(const char* cQuery, const char* cSerialized) {
  Json::Value cBindings;
  Json::CharReaderBuilder reader;
  std::string errors;

  std::istringstream iss(cSerialized);
  if (!Json::parseFromStream(reader, iss, &cBindings, &errors)) {
    std::cout << "[ArnelifyORM FFI]: C error: Invalid cBindings." << std::endl;
    exit(1);
  }

  std::vector<std::string> bindings;
  for (int i = 0; cBindings.size() > i; ++i) {
    const Json::Value& value = cBindings[i];
    bindings.emplace_back(value.asString());
  }

  ArnelifyORMRes res = orm->exec(cQuery, bindings);
  Json::Value json = orm->toJson(res);

  Json::StreamWriterBuilder writer;
  writer["indentation"] = "";
  writer["emitUTF8"] = true;

  const std::string out = Json::writeString(writer, json);
  char* cRes = new char[out.length() + 1];
  std::strcpy(cRes, out.c_str());
  return cRes;
}

void orm_free(const char* cPtr) {
  if (cPtr) delete[] cPtr;
}
}

#endif