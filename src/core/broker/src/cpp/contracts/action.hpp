#ifndef ARNELIFY_BROKER_ACTION_HPP
#define ARNELIFY_BROKER_ACTION_HPP

#include <functional>

#include "json.h"

using BrokerAction = std::function<Json::Value(const Json::Value&)>;

#endif