#ifndef ARNELIFY_ROUTER_FFI_CPP
#define ARNELIFY_ROUTER_FFI_CPP

#include "index.cpp"

extern "C" {

ArnelifyRouter* router = nullptr;

void router_create() { router = new ArnelifyRouter(); }

void router_destroy() { router = nullptr; }

void router_any(char* cPattern) {
  router->any(cPattern, [](const Json::Value& ctx) -> Json::Value {
    return Json::nullValue;
  });
}

void router_get(char* cPattern) {
  router->get(cPattern, [](const Json::Value& ctx) -> Json::Value {
    return Json::nullValue;
  });
}

void router_post(char* cPattern) {
  router->post(cPattern, [](const Json::Value& ctx) -> Json::Value {
    return Json::nullValue;
  });
}

void router_put(char* cPattern) {
  router->put(cPattern, [](const Json::Value& ctx) -> Json::Value {
    return Json::nullValue;
  });
}

void router_patch(char* cPattern) {
  router->patch(cPattern, [](const Json::Value& ctx) -> Json::Value {
    return Json::nullValue;
  });
}

void router_delete(char* cPattern) {
  router->delete_(cPattern, [](const Json::Value& ctx) -> Json::Value {
    return Json::nullValue;
  });
}

const char* router_find(char* cMethod, char* cPath) {
  const std::optional<Route>& routeOpt = router->find(cMethod, cPath);
  if (!routeOpt) {
    char* cRouteOpt = new char[3];
    std::strcpy(cRouteOpt, "{}");
    return cRouteOpt;
  }

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
  char* cRouteOpt = new char[serialized.size() + 1];
  std::strcpy(cRouteOpt, serialized.c_str());
  return cRouteOpt;
}

void router_free(const char* cPointer) {
  if (cPointer) delete[] cPointer;
}

void router_reset() { router->reset(); }
}

#endif