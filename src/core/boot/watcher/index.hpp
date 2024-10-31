#ifndef WATCHER_HPP
#define WATCHER_HPP

#include <csignal>
#include <iostream>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

#include "contracts/path.h"
#include "env/index.hpp"
#include "logger/index.hpp"

class Watcher {
 private:
  pid_t pid = -1;

 public:
  const void apply(const WatcherPath &envPath) {
    Env env;
    env.apply(envPath);
  }

  const void build(const WatcherPath &watchPath, const WatcherPath &serverPath) {
    const std::string buildPath = serverPath.parent_path().string();
    const bool hasBuildPath = std::filesystem::exists(buildPath);
    if (!hasBuildPath) std::filesystem::create_directory(buildPath);
    const std::string srcPath = watchPath.parent_path().string();
    const std::string arg =
        watchPath.filename() == "watch.cpp" ? "watch" : "build";

    Logger::warning("Compiling from sources '" + watchPath.string() + "'");
    const std::string command = "make -C " + srcPath + " " + arg + " -s";
    int ret = system(command.c_str());
    if (ret != 0) exit(1);
  }

  void start(const WatcherPath &serverPath) {
    this->pid = fork();
    const bool isSuccess = this->pid == 0;
    const bool isError = 0 > this->pid;

    if (isSuccess) {
      execl(serverPath.c_str(), serverPath.c_str(), nullptr);
    } else if (isError) {
      std::cout << "Can't create child process." << std::endl;
      exit(1);
    }
  }

  void close() {
    const bool hasPid = this->pid > 0;
    if (hasPid) {
      kill(this->pid, SIGTERM);
      waitpid(this->pid, nullptr, 0);
      this->pid = -1;
    }
  }

  void watch(const WatcherPath &srcPath, const WatcherPath &watchPath,
             const WatcherPath &serverPath) {
    std::thread thread([&]() {
      const std::string command =
          "inotifywait -m -q -r -e modify --exclude './src/storage' " +
          srcPath.string();
      FILE *inotify_pipe = popen(command.c_str(), "r");
      if (!inotify_pipe) {
        Logger::danger("Can't start watcher.");
        exit(1);
      }

      char buffer[256];
      while (fgets(buffer, sizeof(buffer), inotify_pipe)) {
        this->close();
        this->build(watchPath, serverPath);
        this->start(serverPath);
      }

      pclose(inotify_pipe);
    });

    thread.join();
  }
};

#endif