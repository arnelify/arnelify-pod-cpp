#ifndef BOOT_CPP
#define BOOT_CPP

#include <cstring>
#include <iostream>
#include <thread>

#include "logger/index.cpp"
#include "plant/index.cpp"
#include "watcher/index.cpp"

class Boot {
 public:
  static const void setup() {
    Logger::warning("Installing framework...\n");

    Plant::mkdir("./src/app/middleware");
    Plant::mkdir("./src/app/repositories");
    Plant::mkdir("./src/app/requests");
    Plant::mkdir("./src/app/services");
    Plant::mkdir("./src/app/translations");
    Plant::mkdir("./src/database/factories");
    Plant::mkdir("./src/database/migrations");
    Plant::mkdir("./src/database/seeds");
    Plant::mkdir("./src/tests");

    Logger::warning("Installing packages: 'arnelify-server@0.6.3'...", 128);
    system("make -C ./src/core/server build > /dev/null 2>&1");

    Logger::warning("Installing packages: 'arnelify-router@0.5.8'...", 128);
    system("make -C ./src/core/router build > /dev/null 2>&1");

    Logger::warning("Installing packages: 'arnelify-broker@0.5.8'...", 128);
    system("make -C ./src/core/broker build > /dev/null 2>&1");

    Logger::warning("Installing packages: 'arnelify-orm@0.6.1'...", 128);
    system("make -C ./src/core/broker build > /dev/null 2>&1");

    Logger::warning("Installing packages...\n", 128);
    Logger::success("Successfully!\n");
  }

  static const void build() {
    const WatcherPath envPath = ".env";
    const WatcherPath watchPath = "src/server.cpp";
    const WatcherPath serverPath = "pod/server";

    Watcher watcher;
    watcher.apply(envPath);
    watcher.build(watchPath, serverPath);
  }

  static const void watch() {
    const WatcherPath envPath = ".env";
    const WatcherPath watchPath = "src/watch.cpp";
    const WatcherPath serverPath = "pod/server";

    Watcher watcher;
    watcher.apply(envPath);
    watcher.build(watchPath, serverPath);
    watcher.start(serverPath);
    watcher.watch(watchPath, serverPath);
  }

  static const void migrate() {
    Logger::danger("This feature will be available in future versions.\n");
  }

  static const void seed() {
    Logger::danger("This feature will be available in future versions.\n");
  }
};

int main(int argc, char* argv[]) {
  for (int i = 0; argc > i; ++i) {
    const bool isSetup = strcmp(argv[i], "setup") == 0;
    if (isSetup) {
      Boot::setup();
      break;
    }

    const bool isBuild = strcmp(argv[i], "build") == 0;
    if (isBuild) {
      Boot::build();
      break;
    }

    const bool isWatch = strcmp(argv[i], "watch") == 0;
    if (isWatch) {
      Boot::watch();
      break;
    }

    const bool isMigrate = strcmp(argv[i], "migrate") == 0;
    if (isMigrate) {
      Boot::migrate();
      break;
    }

    const bool isSeed = strcmp(argv[i], "seed") == 0;
    if (isSeed) {
      Boot::seed();
      break;
    }
  }

  return 0;
}

#endif