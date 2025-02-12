#ifndef ARNELIFY_ROUTER_CONTROLLER_HPP
#define ARNELIFY_ROUTER_CONTROLLER_HPP

#include <functional>

#include "json.h"

using Ctx = Json::Value;
using Controller = std::function<Json::Value(const Ctx&)>;

#endif