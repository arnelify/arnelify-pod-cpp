#ifndef ROUTER_CALLBACK_CONTRACT_H
#define ROUTER_CALLBACK_CONTRACT_H

#include <functional>
#include <optional>

#include "json.h"

struct Route;

using RouterCallback = std::function<void(const std::optional<Route>)>;

#endif