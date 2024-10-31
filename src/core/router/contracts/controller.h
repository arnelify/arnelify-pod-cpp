#ifndef ROUTER_CONTROLLER_CONTRACT_H
#define ROUTER_CONTROLLER_CONTRACT_H

#include <functional>

#include "json.h"

using RouterCtx = Json::Value;
using Controller = std::function<Json::Value(const RouterCtx&)>;

#endif