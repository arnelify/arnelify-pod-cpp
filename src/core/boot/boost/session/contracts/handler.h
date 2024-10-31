#ifndef BOOST_HANDLER_CONTRACT_H
#define BOOST_HANDLER_CONTRACT_H

#include <functional>

#include "boost/beast.hpp"
#include "json.h"

using BoostReq = Json::Value;
using BoostRawReq =
    boost::beast::http::message<true, boost::beast::http::string_body,
                                boost::beast::http::fields>;

using BoostRes = boost::beast::http::response<boost::beast::http::string_body>;
using BoostRawRes = Json::Value;

using BoostNext = std::function<void(BoostRawRes&)>;
using BoostHandler = std::function<void(const BoostReq&, BoostRes&)>;
using BoostMiddleware = std::function<void(const BoostRawReq&, BoostNext)>;

#endif