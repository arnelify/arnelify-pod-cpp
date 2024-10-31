#ifndef BROKER_CALLBACK_CONTRACT_H
#define BROKER_CALLBACK_CONTRACT_H

#include <functional>

#include "json.h"

using BrokerRequest = std::function<void(const std::string&)>;
using BrokerResponse = std::function<void(const Json::Value&)>;

#endif