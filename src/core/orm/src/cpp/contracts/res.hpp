#ifndef ARNELIFY_ORM_RES_HPP
#define ARNELIFY_ORM_RES_HPP

#include <iostream>
#include <map>
#include <variant>

using ArnelifyORMRow = std::map<std::string, std::variant<std::nullptr_t, std::string>>;
using ArnelifyORMRes = std::vector<ArnelifyORMRow>;

#endif