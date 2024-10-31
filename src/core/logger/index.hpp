#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <iostream>

class Logger {
 public:
  static const void primary(const std::string &message) {
    std::cout << "\033[0m" << "[Arnelify POD]: " << message << "\033[0m"
              << std::endl;
  }

  static const void success(const std::string &message) {
    std::cout << "\033[32m" << "[Arnelify POD]: " << message << "\033[0m"
              << std::endl;
  }

  static const void warning(const std::string &message) {
    std::cout << "\033[33m" << "[Arnelify POD]: " << message << "\033[0m"
              << std::endl;
  }

  static const void danger(const std::string &message) {
    std::cout << "\033[31m" << "[Arnelify POD]: " << message << "\033[0m"
              << std::endl;
  }
};

#endif