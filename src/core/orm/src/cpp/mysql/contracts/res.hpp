#ifndef MYSQL_DRIVER_RES_HPP
#define MYSQL_DRIVER_RES_HPP

#include <iostream>
#include <map>
#include <variant>

using MySQLDriverRow =
    std::map<std::string, std::variant<std::nullptr_t, std::string>>;
using MySQLDriverRes = std::vector<MySQLDriverRow>;

#endif