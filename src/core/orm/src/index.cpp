#ifndef ARNELIFY_ORM_CPP
#define ARNELIFY_ORM_CPP

#include <dlfcn.h>
#include <filesystem>
#include <functional>
#include <iostream>
#include <optional>
#include <stdexcept>

#include "json.h"

#include "cpp/contracts/opts.hpp"
#include "cpp/contracts/res.hpp"

class ArnelifyORM {
 private:
  bool hasHaving;
  bool hasOn;
  bool hasWhere;

  Json::Value opts;
  std::vector<std::string> bindings;
  std::string tableName;
  std::vector<std::string> columns;
  std::vector<std::string> indexes;
  std::string query;

  std::function<void(const std::string&, const bool&)> logger =
      [](const std::string& message, const bool& isError) {
        if (isError) {
          std::cout << "[Arnelify ORM]: Error" << message << std::endl;
          return;
        }

        std::cout << "[Arnelify ORM]: " << message << std::endl;
      };

  void* lib = nullptr;
  void (*orm_create)(const char*);
  void (*orm_destroy)();
  const char* (*orm_exec)(const char*, const char*);
  void (*orm_free)(const char*);

  template <typename T>
  void loadFunction(const std::string& name, T& func) {
    func = reinterpret_cast<T>(dlsym(this->lib, name.c_str()));
    if (!func) {
      throw std::runtime_error(dlerror());
    }
  }

  const std::string getLibPath() {
    const std::filesystem::path scriptDir =
        std::filesystem::absolute(__FILE__).parent_path();

    const bool hasExitSegment = scriptDir.string().ends_with("..");
    if (!hasExitSegment) {
      std::filesystem::path libDir = scriptDir.parent_path();
      const std::string libPath = libDir / "build" / "index.so";
      return libPath;
    }

    std::istringstream stream(scriptDir);
    std::deque<std::string> segments;
    std::string segment;

    while (std::getline(stream, segment, '/')) {
      if (segment.empty()) continue;

      if (segment == "..") {
        if (!segments.empty()) {
          segments.pop_back();
          segments.pop_back();
        }
        continue;
      }

      segments.push_back(segment);
    }

    std::string libPath;
    for (const auto& segment : segments) {
      libPath += "/" + segment;
    }

    libPath += "/build/index.so";
    return libPath;
  }

  const bool isOperator(
      const std::variant<std::nullptr_t, int, double, std::string> arg) {
    if (!std::holds_alternative<std::string>(arg)) return false;
    const std::vector<std::string> operators = {
        "=", "!=", "<=", ">=", "<", ">", "IN", "BETWEEN", "LIKE", "<>"};
    const std::string& operator_ = std::get<std::string>(arg);
    auto it = std::find(operators.begin(), operators.end(), operator_);
    if (it != operators.end()) return true;
    return false;
  }

  const void condition(
      const std::string& column,
      const std::variant<std::nullptr_t, int, double, std::string>& arg2,
      const std::variant<std::nullptr_t, int, double, std::string>& arg3) {
    if (this->isOperator(arg2)) {
      const std::string operator_ = std::get<std::string>(arg2);
      if (std::holds_alternative<std::nullptr_t>(arg3)) {
        this->query += column + " IS NULL";
        return;
      }

      if (std::holds_alternative<int>(arg3)) {
        const std::string value = std::to_string(std::get<int>(arg3));
        this->query += column + " " + operator_ + " " + value;
        this->bindings.emplace_back(value);
        return;
      }

      if (std::holds_alternative<double>(arg3)) {
        const std::string value = std::to_string(std::get<double>(arg3));
        this->query += column + " " + operator_ + " " + value;
        this->bindings.emplace_back(value);
        return;
      }

      const std::string value = std::get<std::string>(arg3);
      this->query += column + " " + operator_ + " ?";
      this->bindings.emplace_back(value);
      return;
    }

    if (std::holds_alternative<std::nullptr_t>(arg2)) {
      this->query += column + " IS NULL";
      return;
    }

    if (std::holds_alternative<int>(arg2)) {
      const std::string value = std::to_string(std::get<int>(arg2));
      this->query += column + " = ?";
      this->bindings.emplace_back(value);
      return;
    }

    if (std::holds_alternative<double>(arg2)) {
      const std::string value = std::to_string(std::get<double>(arg2));
      this->query += column + " = ?";
      this->bindings.emplace_back(value);
      return;
    }

    const std::string value = std::get<std::string>(arg2);
    this->query += column + " = ?";
    this->bindings.emplace_back(value);
  }

 public:
  ArnelifyORM(Json::Value& o)
      : hasHaving(false), hasOn(false), hasWhere(false), opts(o) {
    const std::string libPath = this->getLibPath();
    this->lib = dlopen(libPath.c_str(), RTLD_LAZY);
    if (!this->lib) throw std::runtime_error(dlerror());

    loadFunction("orm_create", this->orm_create);
    loadFunction("orm_destroy", this->orm_destroy);
    loadFunction("orm_exec", this->orm_exec);
    loadFunction("orm_free", this->orm_free);

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    writer["emitUTF8"] = true;

    const std::string cOpts = Json::writeString(writer, this->opts);
    this->orm_create(cOpts.c_str());
  }

  ~ArnelifyORM() {
    if (!this->lib) return;
    this->orm_destroy();
    dlclose(this->lib);
    this->lib = nullptr;
  }

  void alterTable(
      const std::string& tableName,
      const std::function<void(ArnelifyORM*)>& condition =
          [](ArnelifyORM* query) {}) {
    this->query = "ALTER TABLE " + tableName + " ";
    condition(this);

    for (size_t i = 0; this->columns.size() > i; i++) {
      if (i > 0) this->query += ", ";
      this->query += this->columns[i];
    }

    if (this->indexes.size()) this->query += ", ";
    for (size_t i = 0; this->indexes.size() > i; i++) {
      if (i > 0) this->query += ", ";
      this->query += this->indexes[i];
    }

    this->exec();
  }

  void column(const std::string& name, const std::string& type,
              const std::variant<std::nullptr_t, int, double, bool,
                                 std::string>& default_ = false,
              const std::optional<std::string>& after = std::nullopt,
              const std::optional<std::string>& collation = std::nullopt) {
    std::string query = name + " " + type;
    const bool isAlter = this->query.starts_with("ALTER");
    if (isAlter) query = "ADD COLUMN " + name + " " + type;
    if (std::holds_alternative<std::nullptr_t>(default_)) {
      query += " DEFAULT NULL";
    } else if (std::holds_alternative<bool>(default_)) {
      const bool value = std::get<bool>(default_);
      query += " " + std::string(value ? "DEFAULT NULL" : "NOT NULL");
    } else if (std::holds_alternative<int>(default_)) {
      query += std::to_string(std::get<int>(default_));
    } else if (std::holds_alternative<std::string>(default_)) {
      const std::string value = std::get<std::string>(default_);
      if (value == "CURRENT_TIMESTAMP") {
        query += " DEFAULT CURRENT_TIMESTAMP";
      } else {
        query += "'" + std::get<std::string>(default_) + "'";
      }
    }

    if (collation.has_value()) query += " COLLATE " + collation.value();
    if (after.has_value()) query += " AFTER " + after.value();
    this->columns.emplace_back(query);
  }

  void createTable(
      const std::string& tableName,
      const std::function<void(ArnelifyORM*)>& condition =
          [](ArnelifyORM* query) {}) {
    this->query = "CREATE TABLE " + tableName + " (";
    condition(this);
    for (size_t i = 0; this->columns.size() > i; i++) {
      if (i > 0) this->query += ", ";
      this->query += this->columns[i];
    }

    if (this->indexes.size()) this->query += ", ";
    for (size_t i = 0; this->indexes.size() > i; i++) {
      if (i > 0) this->query += ", ";
      this->query += this->indexes[i];
    }

    this->query += ")";
    this->exec();
  }

  ArnelifyORM* delete_() {
    this->query = "DELETE FROM " + this->tableName;
    return this;
  }

  ArnelifyORM* distinct(const std::vector<std::string>& args = {}) {
    if (!args.size()) {
      this->query = "SELECT DISTINCT * FROM " + this->tableName;
      return this;
    }

    this->query = "SELECT DISTINCT ";
    for (size_t i = 0; args.size() > i; i++) {
      if (i > 0) this->query += ", ";
      this->query += args[i];
    }

    this->query += " FROM " + this->tableName;
    return this;
  }

  void dropColumn(const std::string& name,
                  const std::vector<std::string> args = {}) {
    std::string query = "DROP COLUMN " + name;
    for (size_t i = 0; args.size() > i; i++) {
      query += " " + args[i];
    }

    this->columns.emplace_back(query);
  }

  void dropConstraint(std::string& name) {
    this->query += "DROP CONSTRAINT " + name;
  }

  void dropIndex(const std::string& name) {
    this->query += "DROP INDEX " + name;
  }

  void dropTable(const std::string& tableName,
                 const std::vector<std::string> args = {}) {
    this->raw("SET foreign_key_checks = 0;");
    this->query = "DROP TABLE IF EXISTS " + tableName;
    for (size_t i = 0; args.size() > i; i++) {
      this->query += " " + args[i];
    }

    this->exec();
    this->raw("SET foreign_key_checks = 1;");
  }

  const ArnelifyORMRes exec(const std::string& query,
                            const std::vector<std::string>& bindings) {
    Json::Value bindingsJson = Json::arrayValue;
    for (int i = 0; bindings.size() > i; ++i) {
      bindingsJson.append(bindings[i]);
    }

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    writer["emitUTF8"] = true;

    const std::string bindingsString = Json::writeString(writer, bindingsJson);
    const char* cRes = this->orm_exec(query.c_str(), bindingsString.c_str());
    const std::string serialized = cRes;

    Json::Value resJson;
    Json::CharReaderBuilder reader;
    std::string errors;

    std::istringstream iss(serialized);
    if (!Json::parseFromStream(reader, iss, &resJson, &errors)) {
      std::cout << "[ArnelifyORM FFI]: C++ error: cRes must be in valid JSON."
                << std::endl;
      exit(1);
    }

    this->orm_free(cRes);

    ArnelifyORMRes res;
    for (int i = 0; resJson.size() > i; ++i) {
      ArnelifyORMRow row;
      const Json::Value& item = resJson[i];
      for (const std::string& key : item.getMemberNames()) {
        const Json::Value& value = item[key];
        if (value.isNull()) {
          row[key] = nullptr;
          continue;
        }

        row[key] = value.asString();
      }

      res.emplace_back(row);
    }

    return res;
  }

  const ArnelifyORMRes exec() {
    Json::Value bindingsJson = Json::arrayValue;
    for (int i = 0; this->bindings.size() > i; ++i) {
      bindingsJson.append(this->bindings[i]);
    }

    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    writer["emitUTF8"] = true;

    const std::string bindingsString = Json::writeString(writer, bindingsJson);
    const char* cRes = this->orm_exec(this->query.c_str(), bindingsString.c_str());
    const std::string serialized = cRes;

    Json::Value resJson;
    Json::CharReaderBuilder reader;
    std::string errors;
    std::istringstream iss(serialized);
    if (!Json::parseFromStream(reader, iss, &resJson, &errors)) {
      std::cout << "[ArnelifyORM FFI]: C++ error: cRes must be in valid JSON."
                << std::endl;
      exit(1);
    }

    this->orm_free(cRes);

    ArnelifyORMRes res;
    for (int i = 0; resJson.size() > i; ++i) {
      ArnelifyORMRow row;
      const Json::Value& item = resJson[i];
      for (const std::string& key : item.getMemberNames()) {
        const Json::Value& value = item[key];
        if (value.isNull()) {
          row[key] = nullptr;
          continue;
        }

        row[key] = value.asString();
      }

      res.emplace_back(row);
    }

    this->hasHaving = false;
    this->hasOn = false;
    this->hasWhere = false;

    this->bindings.clear();
    this->tableName.clear();
    this->columns.clear();
    this->indexes.clear();
    this->query.clear();

    return res;
  }

  ArnelifyORM* groupBy(const std::vector<std::string>& args) {
    this->query += " GROUP BY ";
    for (size_t i = 0; args.size() > i; i++) {
      if (i > 0) this->query += ", ";
      this->query += args[i];
    }

    return this;
  }

  ArnelifyORM* having(const std::function<void(ArnelifyORM*)>& condition) {
    if (this->hasHaving) {
      const bool hasCondition = this->query.ends_with(")");
      if (hasCondition) this->query += " AND ";
    } else {
      this->query += " HAVING ";
      this->hasHaving = true;
    }

    this->query += "(";
    condition(this);
    this->query += ")";
    return this;
  }

  ArnelifyORM* having(
      const std::string& column,
      const std::variant<std::nullptr_t, int, double, std::string>& arg2,
      const std::variant<std::nullptr_t, int, double, std::string>& arg3 =
          nullptr) {
    if (this->hasHaving) {
      const bool hasCondition = this->query.ends_with("?");
      if (hasCondition) this->query += " AND ";
    } else {
      this->query += " HAVING ";
      this->hasHaving = true;
    }

    this->condition(column, arg2, arg3);
    return this;
  }

  ArnelifyORMRes insert(
      const std::map<
          std::string,
          const std::variant<std::nullptr_t, int, double, std::string>>& args) {
    this->query = "INSERT INTO " + this->tableName;
    std::stringstream columns;
    std::stringstream values;

    bool first = true;
    for (const auto& [key, value] : args) {
      if (!first) {
        columns << ", ";
        values << ", ";
        first = false;
      }

      columns << key;
      if (std::holds_alternative<std::nullptr_t>(value)) {
        values << "NULL";
        continue;
      }

      if (std::holds_alternative<int>(value)) {
        const std::string binding = std::to_string(std::get<int>(value));
        this->bindings.emplace_back(binding);
        values << "?";
        continue;
      }

      if (std::holds_alternative<double>(value)) {
        const std::string binding = std::to_string(std::get<double>(value));
        this->bindings.emplace_back(binding);
        values << "?";
        continue;
      }

      if (std::holds_alternative<std::string>(value)) {
        const std::string binding = std::get<std::string>(value);
        this->bindings.emplace_back(binding);
        values << "?";
        continue;
      }
    }

    this->query += " (" + columns.str() + ") VALUES (" + values.str() + ")";
    return this->exec();
  }

  void index(const std::string& type, const std::vector<std::string> args) {
    std::string query = type + " idx";
    const bool isAlter = this->query.starts_with("ALTER");
    if (isAlter) query = "ADD " + type + " idx";

    for (size_t i = 0; args.size() > i; i++) {
      query += "_" + args[i];
    }

    query += " (";
    for (size_t i = 0; args.size() > i; i++) {
      if (i > 0) query += ", ";
      query += args[i];
    }

    query += ")";
    this->indexes.emplace_back(query);
  }

  ArnelifyORM* join(const std::string& tableName) {
    this->query += " JOIN " + tableName;
    return this;
  }

  ArnelifyORMRes limit(const int& limit_, const int& offset = 0) {
    if (offset > 0) {
      this->query +=
          " LIMIT " + std::to_string(offset) + ", " + std::to_string(limit_);
      return this->exec();
    }

    this->query += " LIMIT " + std::to_string(limit_);
    return this->exec();
  }

  ArnelifyORM* leftJoin(const std::string& tableName) {
    this->query += " LEFT JOIN " + tableName;
    return this;
  }

  ArnelifyORM* on(const std::function<void(ArnelifyORM*)>& condition) {
    if (this->hasOn) {
      const bool hasCondition = this->query.ends_with(")");
      if (hasCondition) this->query += " AND ";
    } else {
      this->query += " ON ";
      this->hasOn = true;
    }

    this->query += "(";
    condition(this);
    this->query += ")";
    return this;
  }

  ArnelifyORM* on(
      const std::string& column,
      const std::variant<std::nullptr_t, int, double, std::string>& arg2,
      const std::variant<std::nullptr_t, int, double, std::string>& arg3 =
          nullptr) {
    if (this->hasOn) {
      const bool hasCondition = this->query.ends_with("?");
      if (hasCondition) this->query += " AND ";
    } else {
      this->query += " ON ";
      this->hasOn = true;
    }

    this->condition(column, arg2, arg3);
    return this;
  }

  ArnelifyORM* offset(const int& offset) {
    this->query += " OFFSET " + std::to_string(offset);
    return this;
  }

  ArnelifyORM* orderBy(const std::string& column, const std::string& arg2) {
    this->query += " ORDER BY " + column + " " + arg2;
    return this;
  }

  ArnelifyORM* orHaving(const std::function<void(ArnelifyORM*)>& condition) {
    if (this->hasHaving) {
      const bool hasCondition = this->query.ends_with(")");
      if (hasCondition) this->query += " OR ";
    } else {
      this->query += " HAVING ";
      this->hasHaving = true;
    }

    this->query += "(";
    condition(this);
    this->query += ")";
    return this;
  }

  ArnelifyORM* orHaving(
      const std::string& column,
      const std::variant<std::nullptr_t, int, double, std::string>& arg2,
      const std::variant<std::nullptr_t, int, double, std::string>& arg3 =
          nullptr) {
    if (this->hasHaving) {
      const bool hasCondition = this->query.ends_with("?");
      if (hasCondition) this->query += " OR ";
    } else {
      this->query += " HAVING ";
      this->hasHaving = true;
    }

    this->condition(column, arg2, arg3);
    return this;
  }

  ArnelifyORM* orOn(const std::function<void(ArnelifyORM*)>& condition) {
    if (this->hasOn) {
      const bool hasCondition = this->query.ends_with(")");
      if (hasCondition) this->query += " OR ";
    } else {
      this->query += " ON ";
      this->hasOn = true;
    }

    this->query += "(";
    condition(this);
    this->query += ")";
    return this;
  }

  ArnelifyORM* orOn(
      const std::string& column,
      const std::variant<std::nullptr_t, int, double, std::string>& arg2,
      const std::variant<std::nullptr_t, int, double, std::string>& arg3 =
          nullptr) {
    if (this->hasOn) {
      const bool hasCondition = this->query.ends_with("?");
      if (hasCondition) this->query += " OR ";
    } else {
      this->query += " ON ";
      this->hasOn = true;
    }

    this->condition(column, arg2, arg3);
    return this;
  }

  ArnelifyORM* orWhere(const std::function<void(ArnelifyORM*)>& condition) {
    if (this->hasWhere) {
      const bool hasCondition = this->query.ends_with(")");
      if (hasCondition) this->query += " OR ";
    } else {
      this->query += " WHERE ";
      this->hasWhere = true;
    }

    this->query += "(";
    condition(this);
    this->query += ")";
    return this;
  }

  ArnelifyORM* orWhere(
      const std::string& column,
      const std::variant<std::nullptr_t, int, double, std::string>& arg2,
      const std::variant<std::nullptr_t, int, double, std::string>& arg3 =
          nullptr) {
    if (this->hasWhere) {
      const bool hasCondition = this->query.ends_with("?");
      if (hasCondition) this->query += " OR ";
    } else {
      this->query += " WHERE ";
      this->hasWhere = true;
    }

    this->condition(column, arg2, arg3);
    return this;
  }

  ArnelifyORMRes raw(const std::string& query) {
    this->query = query;
    return this->exec();
  }

  void reference(const std::string& column, const std::string& tableName,
                 const std::string& foreign,
                 const std::vector<std::string> args) {
    std::string query = "CONSTRAINT fk_" + tableName + " FOREIGN KEY (" +
                        column + ") REFERENCES " + tableName + "(" + foreign +
                        ")";

    const bool isAlter = this->query.starts_with("ALTER");
    if (isAlter) {
      query = "ADD CONSTRAINT fk_" + tableName + " FOREIGN KEY (" + column +
              ") REFERENCES " + tableName + "(" + foreign + ")";
    }

    for (size_t i = 0; args.size() > i; i++) {
      query += " " + args[i];
    }

    this->indexes.emplace_back(query);
  }

  ArnelifyORM* rightJoin(const std::string& tableName) {
    this->query += " RIGHT JOIN " + tableName;
    return this;
  }

  ArnelifyORM* select(const std::vector<std::string>& args = {}) {
    if (!args.size()) {
      this->query = "SELECT * FROM " + this->tableName;
      return this;
    }

    this->query = "SELECT ";
    for (size_t i = 0; args.size() > i; i++) {
      if (i > 0) this->query += ", ";
      this->query += args[i];
    }

    this->query += " FROM " + this->tableName;
    return this;
  }

  ArnelifyORM* table(const std::string& tableName) {
    this->tableName = tableName;
    return this;
  }

  const Json::Value toJson(const ArnelifyORMRes& res) {
    Json::Value json = Json::arrayValue;
    for (const ArnelifyORMRow& row : res) {
      Json::Value item;

      for (auto& [key, value] : row) {
        if (std::holds_alternative<std::nullptr_t>(value)) {
          item[key] = Json::nullValue;
          continue;
        }

        item[key] = std::get<std::string>(value);
      }

      json.append(item);
    }

    return json;
  }

  ArnelifyORM* update(
      const std::map<
          std::string,
          const std::variant<std::nullptr_t, int, double, std::string>>& args) {
    this->query = "UPDATE ";
    this->query += this->tableName;
    this->query += " SET ";

    bool first = true;
    for (const auto& [key, value] : args) {
      if (!first) {
        this->query += ", ";
        first = false;
      }

      if (std::holds_alternative<std::nullptr_t>(value)) {
        this->query += key + " IS NULL";
        continue;
      }

      if (std::holds_alternative<int>(value)) {
        const std::string binding = std::to_string(std::get<int>(value));
        this->bindings.emplace_back(binding);
        this->query += key + " = ?";
        continue;
      }

      if (std::holds_alternative<double>(value)) {
        const std::string binding = std::to_string(std::get<double>(value));
        this->bindings.emplace_back(binding);
        this->query += key + " = ?";
        continue;
      }

      if (std::holds_alternative<std::string>(value)) {
        const std::string binding = std::get<std::string>(value);
        this->bindings.emplace_back(binding);
        this->query += key + " = ?";
        continue;
      }
    }

    return this;
  }

  ArnelifyORM* where(const std::function<void(ArnelifyORM*)>& condition) {
    if (this->hasWhere) {
      const bool hasCondition = this->query.ends_with(")");
      if (hasCondition) this->query += " AND ";
    } else {
      this->query += " WHERE ";
      this->hasWhere = true;
    }

    this->query += "(";
    condition(this);
    this->query += ")";
    return this;
  }

  ArnelifyORM* where(
      const std::string& column,
      const std::variant<std::nullptr_t, int, double, std::string>& arg2,
      const std::variant<std::nullptr_t, int, double, std::string>& arg3 =
          nullptr) {
    if (this->hasWhere) {
      const bool hasCondition = this->query.ends_with("?");
      if (hasCondition) this->query += " AND ";
    } else {
      this->query += " WHERE ";
      this->hasWhere = true;
    }

    this->condition(column, arg2, arg3);
    return this;
  }
};

#endif