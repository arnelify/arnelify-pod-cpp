#ifndef PLANT_CPP
#define PLANT_CPP

#include <filesystem>
#include <iostream>

#include "logger/index.cpp"

class Plant {
 public:
  static const void xcopy(const std::string& srcPath,
                          const std::string& destPath) {
    const bool isSrcExists = std::filesystem::exists(srcPath);
    if (!isSrcExists) {
      Logger::danger("Source path does not exist.");
      exit(1);
    }

    if (std::filesystem::is_directory(srcPath)) {
      std::filesystem::create_directories(destPath);
      for (const auto& entry : std::filesystem::directory_iterator(srcPath)) {
        const auto& srcFile = entry.path();
        const auto destFile =
            std::filesystem::path(destPath) / srcFile.filename();
        Plant::xcopy(srcFile.string(), destFile.string());
      }
    } else {
      std::filesystem::copy(srcPath, destPath,
                            std::filesystem::copy_options::overwrite_existing);
    }
  }

  static const void mkdir(const std::string& rawPath) {
    const std::filesystem::path path = rawPath;
    const bool hasPath = std::filesystem::exists(path);
    if (!hasPath) std::filesystem::create_directory(path);
  }
};

#endif