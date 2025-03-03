#ifndef ARNELIFY_ORM_TEST_CPP
#define ARNELIFY_ORM_TEST_CPP

#include <iostream>

#include "json.h"

#include "../index.cpp"

int main(int argc, char* argv[]) {

  Json::Value opts;
  opts["ORM_DRIVER"] = "mysql";
  opts["ORM_HOST"] = "mysql";
  opts["ORM_NAME"] = "test";
  opts["ORM_USER"] = "root";
  opts["ORM_PASS"] = "pass";
  opts["ORM_PORT"] = 3306;

  ArnelifyORM* db = new ArnelifyORM(opts);
  ArnelifyORMRes res;

  Json::StreamWriterBuilder writer;
  writer["indentation"] = "";
  writer["emitUTF8"] = true;

  db->dropTable("users");
  db->dropTable("posts");

  db->createTable("users", [](ArnelifyORM* query){
    query->column("id", "BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY");
    query->column("email", "VARCHAR(255) UNIQUE", nullptr);
    query->column("created_at", "DATETIME", "CURRENT_TIMESTAMP");
    query->column("updated_at", "DATETIME", nullptr);
  });

  db->createTable("posts", [](ArnelifyORM* query){
    query->column("id", "BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY");
    query->column("user_id", "BIGINT UNSIGNED", nullptr);
    query->column("contents", "VARCHAR(2048)", nullptr);
    query->column("created_at", "DATETIME", "CURRENT_TIMESTAMP");
    query->column("updated_at", "DATETIME", "CURRENT_TIMESTAMP");

    query->index("INDEX", {"user_id"});
    query->reference("user_id", "users", "id", {"ON DELETE CASCADE"});
  });
  
  res = db->table("users")
    ->insert({{"email", "email@example.com"}});

  Json::Value insert = db->toJson(res);
  std::cout << "last inserted id: " << Json::writeString(writer, insert) << std::endl;

  res = db->table("users")
    ->select({"id", "email"})
    ->where("id", 1)
    ->limit(1);

  Json::Value select = db->toJson(res);
  std::cout << "inserted row: " << Json::writeString(writer, select) << std::endl;

  db->table("users")
    ->update({{"email", "user@example.com"}})
    ->where("id", 1)
    ->exec();

  db->table("users")
    ->delete_()
    ->where("id", 1)
    ->limit(1);

  return 0;
}

#endif