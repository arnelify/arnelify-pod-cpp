#ifndef ARNELIFY_SERVER_TEST_CPP
#define ARNELIFY_SERVER_TEST_CPP

#include <iostream>

#include "json.h"

#include "../index.cpp"

int main(int argc, char* argv[]) {
  
  Json::Value opts;
  opts["SERVER_ALLOW_EMPTY_FILES"] = true;
  opts["SERVER_BLOCK_SIZE_KB"] = 64;
  opts["SERVER_CHARSET"] = "UTF-8";
  opts["SERVER_GZIP"] = true;
  opts["SERVER_KEEP_EXTENSIONS"] = true;
  opts["SERVER_MAX_FIELDS"] = 1024;
  opts["SERVER_MAX_FIELDS_SIZE_TOTAL_MB"] = 20;
  opts["SERVER_MAX_FILES"] = 1;
  opts["SERVER_MAX_FILES_SIZE_TOTAL_MB"] = 60;
  opts["SERVER_MAX_FILE_SIZE_MB"] = 60;
  opts["SERVER_PORT"] = 3001;
  opts["SERVER_QUEUE_LIMIT"] = 1024;
  opts["SERVER_UPLOAD_PATH"] = "./src/storage/upload";

  ArnelifyServer server(opts);

  server.setHandler([](const ArnelifyServerReq& req, ArnelifyServerRes& res) {
    Json::StreamWriterBuilder writer;
    writer["indentation"] = "";
    writer["emitUTF8"] = true;

    res.setCode(200);
    res.addBody(Json::writeString(writer, req));
    res.end();
  });
  
  server.start([](const std::string& message, const bool& isError) {
    if (isError) {
      std::cout << "[Arnelify Server]: Error: " << message << std::endl;
      exit(1);
    }

    std::cout << "[Arnelify Server]: " << message << std::endl;
  });

  return 0;
}

#endif