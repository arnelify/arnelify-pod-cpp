#ifndef BROKER_ACTION_CONTRACT_H
#define BROKER_ACTION_CONTRACT_H

#include <functional>

#include "json.h"

using BrokerCtx = Json::Value;
using BrokerAction = std::function<Json::Value(const BrokerCtx&)>;

#endif