#ifndef BOOST_PARSER_CONTRACT_H
#define BOOST_PARSER_CONTRACT_H

#include "boost/beast.hpp"

using BoostParser =
    boost::beast::http::request_parser<boost::beast::http::string_body>;

#endif