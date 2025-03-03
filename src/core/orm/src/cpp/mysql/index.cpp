#ifndef MYSQL_DRIVER_CPP
#define MYSQL_DRIVER_CPP

#include <iostream>
#include <map>

#include "mysql/mysql.h"

#include "contracts/logger.hpp"
#include "contracts/res.hpp"

using my_bool = my_bool;

class MySQLDriver {
 private:
  MYSQL* mysql;

  MySQLDriverLogger logger = [](const std::string& message,
                                const bool& isError) {
    if (isError) {
      std::cout << "[Arnelify ORM]: Error: " << message << std::endl;
      return;
    }

    std::cout << "[Arnelify ORM]: " << message << std::endl;
  };

 public:
  MySQLDriver(const std::string& host, const std::string& name,
              const std::string& user, const std::string& pass,
              const int& port) {
    this->mysql = mysql_init(NULL);
    if (this->mysql == NULL) {
      this->logger("MySQL init failed.", true);
      exit(1);
    }

    if (mysql_real_connect(this->mysql, host.c_str(), user.c_str(),
                           pass.c_str(), name.c_str(), 0, NULL, 0) == NULL) {
      this->logger("MySQL connection failed.", true);
      mysql_close(this->mysql);
      exit(1);
    }
  }

  ~MySQLDriver() {
    if (this->mysql) mysql_close(this->mysql);
  }

  const MySQLDriverRes exec(const std::string& query,
                            const std::vector<std::string>& bindings) {
    MYSQL_STMT* stmt = mysql_stmt_init(mysql);
    if (!stmt) {
      this->logger("Failed to initialize prepared statement.", true);
      exit(1);
    }

    if (mysql_stmt_prepare(stmt, query.c_str(), query.length())) {
      this->logger(
          "Failed to prepare statement: " + std::string(mysql_error(mysql)),
          true);
      mysql_stmt_close(stmt);
      exit(1);
    }

    int i = 0;
    const std::size_t bindingsLen = bindings.size();
    std::vector<MYSQL_BIND> query_bind(bindingsLen);
    for (const std::string& binding : bindings) {
      if (binding.empty()) {
        query_bind[i].buffer_type = MYSQL_TYPE_NULL;
        query_bind[i].buffer = nullptr;
        query_bind[i].buffer_length = 0;
        query_bind[i].is_null = new my_bool(1);
      } else {
        query_bind[i].buffer_type = MYSQL_TYPE_STRING;
        query_bind[i].buffer = (void*)binding.c_str();
        query_bind[i].buffer_length = binding.size();
        query_bind[i].is_null = new my_bool(0);
      }
      query_bind[i].length = nullptr;
      i++;
    }

    if (mysql_stmt_bind_param(stmt, query_bind.data())) {
      this->logger(
          "Failed to bind parameters: " + std::string(mysql_error(mysql)),
          true);
      mysql_stmt_close(stmt);
      exit(1);
    }

    if (mysql_stmt_execute(stmt)) {
      this->logger(
          "Failed to execute statement: " + std::string(mysql_error(mysql)),
          true);
      mysql_stmt_close(stmt);
      exit(1);
    }

    MySQLDriverRes res;
    for (int i = 0; i < bindingsLen; i++) {
      delete query_bind[i].is_null;
    }

    const bool isInsert = query.starts_with("INSERT");
    if (isInsert) {
      MySQLDriverRow row;
      row["id"] = std::to_string(mysql_insert_id(this->mysql));
      res.emplace_back(row);
    }

    const bool isSelect = query.starts_with("SELECT");
    if (isSelect) {
      MYSQL_RES* result = mysql_stmt_result_metadata(stmt);
      if (result == NULL) {
        this->logger("Failed to retrieve result metadata.", true);
        mysql_stmt_close(stmt);
        exit(1);
      }

      const int numFields = mysql_num_fields(result);
      const MYSQL_FIELD* fields = mysql_fetch_fields(result);
      std::vector<MYSQL_BIND> result_bind(numFields);
      std::vector<unsigned long> result_lengths(numFields);
      std::vector<char*> result_buffer(numFields);
      for (i = 0; i < numFields; i++) {
        result_buffer[i] = new char[result_lengths[i] + 1];
        result_lengths[i] = fields[i].length;
        result_bind[i].buffer = result_buffer[i];
        result_bind[i].buffer_length = result_lengths[i] + 1;
        result_bind[i].length = &result_lengths[i];
        result_bind[i].is_null = new my_bool(0);
      }

      if (mysql_stmt_bind_result(stmt, result_bind.data())) {
        this->logger(
            "Failed to bind result: " + std::string(mysql_error(mysql)), true);
        mysql_stmt_close(stmt);
        exit(1);
      }

      while (mysql_stmt_fetch(stmt) == 0) {
        MySQLDriverRow row;
        
        for (int i = 0; i < numFields; i++) {
          const bool isNull = *(result_bind[i].is_null);
          const std::string key = fields[i].name;
          if (isNull) {
            row[key] = nullptr;
            continue;
          }

          const unsigned long index = result_lengths[i];
          result_buffer[i][index] = '\0';
          row[key] = result_buffer[i];
        }

        res.emplace_back(row);
      }

      for (int i = 0; i < numFields; i++) {
        delete result_bind[i].is_null;
        delete[] result_buffer[i];
      }

      mysql_free_result(result);
    }

    mysql_stmt_close(stmt);
    return res;
  }
};

#endif