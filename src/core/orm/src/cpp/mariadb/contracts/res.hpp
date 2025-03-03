#ifndef MARIADB_DRIVER_RES_HPP
#define MARIADB_DRIVER_RES_HPP

#include <iostream>
#include <map>
#include <variant>

using MariaDBDriverRes =
    std::map<std::string, std::variant<std::nullptr_t, std::string>>;

#endif