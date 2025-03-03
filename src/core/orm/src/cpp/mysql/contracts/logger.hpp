#ifndef MYSQL_DRIVER_LOGGER_HPP
#define MYSQL_DRIVER_LOGGER_HPP

#include <iostream>
#include <functional>

using MySQLDriverLogger = std::function<void(const std::string&, const bool&)>;

#endif