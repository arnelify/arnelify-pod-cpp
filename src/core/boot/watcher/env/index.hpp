#ifndef ENV_HPP
#define ENV_HPP

#include <filesystem>
#include <fstream>
#include <map>

#include "contracts/path.h"

class Env {
 private:
  const std::string read(const EnvPath &envPath) {
    std::ifstream stream(envPath);
    const bool isStreamOpen = stream.is_open();
    if (!isStreamOpen) {
      std::cout << "Error opening file: " + envPath.string() << std::endl;
      exit(1);
    }

    std::stringstream buffer;
    buffer << stream.rdbuf();
    const std::string raw = buffer.str();
    stream.close();

    return raw;
  }

  const std::map<std::string, std::string> parse(const std::string &raw) {
    std::map<std::string, std::string> data;
    std::istringstream stream(raw);

    std::string line;
    while (std::getline(stream, line)) {
      line.erase(0, line.find_first_not_of(" \n\r\t"));
      line.erase(line.find_last_not_of(" \n\r\t") + 1);

      const bool isEmpty = line.empty() || line[0] == '#';
      if (isEmpty) continue;

      auto pos = line.find('=');
      bool isEmptyValue = pos == std::string::npos;
      if (isEmptyValue) continue;

      std::string key = line.substr(0, pos);
      std::string value = line.substr(pos + 1);

      key.erase(0, key.find_first_not_of(" \n\r\t"));
      key.erase(key.find_last_not_of(" \n\r\t") + 1);
      value.erase(0, value.find_first_not_of(" \n\r\t"));
      value.erase(value.find_last_not_of(" \n\r\t") + 1);

      const bool hasKey = !key.empty();
      const bool hasValue = !value.empty();
      if (hasKey && hasValue) data[key] = value;
    }

    return data;
  }

  const std::string sources(const std::map<std::string, std::string> &data) {
    std::ostringstream oss;

    oss << "#ifndef ENV_HPP\n";
    oss << "#define ENV_HPP\n\n";
    oss << "#include <iostream>\n\n";

    oss << "class Env {\n";
    oss << " public:\n";

    for (auto &pair : data) {
      oss << "  const std::string " << pair.first << " = \"" << pair.second
          << "\";\n";
    }

    oss << "};\n\n";
    oss << "Env env;\n\n";
    oss << "#endif";

    return oss.str();
  }

  const void save(const EnvPath &envPath, const std::string &data) {
    const EnvPath outDir = envPath.parent_path();

    std::filesystem::create_directories(outDir);
    std::ofstream stream(envPath);

    const bool isStreamOpen = stream.is_open();
    if (!isStreamOpen) {
      std::cerr << "Error opening file: " << envPath << std::endl;
      exit(1);
    }

    stream << data;
    stream.close();
  }

 public:
  const void apply(const EnvPath &envPath) {
    const EnvPath savePath = "./src/core/env/index.hpp";
    const std::string raw = this->read(envPath);
    const std::map<std::string, std::string> data = this->parse(raw);
    const std::string sources = this->sources(data);
    this->save(savePath, sources);
  }
};

#endif