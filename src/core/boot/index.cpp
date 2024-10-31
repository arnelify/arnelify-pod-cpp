#include <cstring>
#include <iostream>
#include <thread>

#include "plant/index.hpp"
#include "watcher/index.hpp"

class Boot {
 public:
  static const void setup() {
    Logger::warning("Installing framework...");

    Plant::mkdir("./src/app/middleware");
    Plant::mkdir("./src/app/repositories");
    Plant::mkdir("./src/app/requests");
    Plant::mkdir("./src/app/services");
    Plant::mkdir("./src/app/translations");
    Plant::mkdir("./src/database/factories");
    Plant::mkdir("./src/database/migrations");
    Plant::mkdir("./src/database/seeds");
    Plant::mkdir("./src/tests");

    Logger::warning("Installing package: 'libboost-all-dev'...");

    Plant::xcopy("/usr/include/boost/", "./include/boost");
    
    Logger::warning("Installing package: 'libjsoncpp-dev'...");

    Plant::xcopy("/usr/include/jsoncpp", "./include/jsoncpp");
    
    Logger::warning("Installing package: 'libmagic-dev'...");
    
    Plant::xcopy("/usr/include/magic.h", "./include/magic.h");

    Logger::success("Successful!");
  }

  static const void build() {
    const WatcherPath envPath = "./.env";
    const WatcherPath watchPath = "./src/server.cpp";
    const WatcherPath serverPath = "./pod/server";

    Watcher watcher;
    watcher.apply(envPath);
    watcher.build(watchPath, serverPath);
  }

  static const void watch() {
    const WatcherPath envPath = "./.env";
    const WatcherPath watchPath = "./src/watch.cpp";
    const WatcherPath serverPath = "./pod/server";
    const WatcherPath srcPath = "./src";

    Watcher watcher;
    watcher.apply(envPath);
    watcher.build(watchPath, serverPath);
    watcher.start(serverPath);
    watcher.watch(srcPath, watchPath, serverPath);
  }

  static const void migrate() {
    Logger::danger("This feature will be available in future versions.");
  }

  static const void seed() {
    Logger::danger("This feature will be available in future versions.");
  }
};

int main(int argc, char *argv[]) {
  for (int i = 0; argc > i; ++i) {
    const bool isSetup = strcmp(argv[i], "setup") == 0;
    if (isSetup) {
      Boot::setup();
      return 0;
    }

    const bool isBuild = strcmp(argv[i], "build") == 0;
    if (isBuild) {
      Boot::build();
      return 0;
    }

    const bool isWatch = strcmp(argv[i], "watch") == 0;
    if (isWatch) {
      Boot::watch();
      return 0;
    }

    const bool isMigrate = strcmp(argv[i], "migrate") == 0;
    if (isMigrate) {
      Boot::migrate();
      return 0;
    }

    const bool isSeed = strcmp(argv[i], "seed") == 0;
    if (isSeed) {
      Boot::seed();
      return 0;
    }
  }

  return 0;
}