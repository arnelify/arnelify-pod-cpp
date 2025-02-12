#ifndef WATCHER_CPP
#define WATCHER_CPP

#include <csignal>
#include <filesystem>
#include <iostream>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

#include "env/index.cpp"
#include "logger/index.cpp"

#include "contracts/path.hpp"

class Watcher {
 private:
  pid_t pid = -1;

 public:
  const void apply(const WatcherPath &envPath) {
    const EnvPath savePath = "src/core/env/index.cpp";
    const std::string raw = Env::read(envPath);
    const std::map<std::string, std::string> data = Env::parse(raw);
    const std::string sources = Env::sources(data);
    Env::save(savePath, sources);
  }

  const void build(const WatcherPath &watchPath,
                   const WatcherPath &serverPath) {
    const std::string buildPath = serverPath.parent_path().string();
    const bool hasBuildPath = std::filesystem::exists(buildPath);
    if (!hasBuildPath) std::filesystem::create_directory(buildPath);

    Logger::warning("Compiling from sources '" + watchPath.string() + "'\n");
    const bool isWatch = watchPath.filename() == "watch.cpp";
    if (isWatch) {
      const std::string command = "cd ./src && make watch -s";
      int ret = system(command.c_str());
      if (ret != 0) exit(1);
      return;
    }

    const std::string command = "cd ./src && make build -s";
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
      Logger::danger("Can't create child process.\n");
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

  void watch(const WatcherPath &watchPath, const WatcherPath &serverPath) {
    const std::filesystem::path srcPath = watchPath.parent_path().string();
    std::thread thread([&]() {
      const std::string command =
          "inotifywait -m -q -r -e modify --exclude "
          "'src/core/env|src/storage' " +
          srcPath.string();
      FILE *inotify_pipe = popen(command.c_str(), "r");
      if (!inotify_pipe) {
        Logger::danger("Can't start watcher.\n");
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