#ifndef MARIADB_DRIVER_LOGGER_HPP
#define MARIADB_DRIVER_LOGGER_HPP

#include <iostream>
#include <functional>

using MariaDBDriverLogger = std::function<void(const std::string&, const bool&)>;

#endif