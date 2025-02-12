#ifndef ARNELIFY_BROKER_CALLBACK_HPP
#define ARNELIFY_BROKER_CALLBACK_HPP

#include <functional>

#include "json.h"

using BrokerRequest = std::function<void(const std::string&)>;
using BrokerResponse = std::function<void(const Json::Value&)>;

#endif