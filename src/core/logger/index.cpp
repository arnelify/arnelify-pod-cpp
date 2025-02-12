#ifndef ARNELIFY_LOGGER_CPP
#define ARNELIFY_LOGGER_CPP

#include <iostream>
#include <iomanip>

class Logger {
 private:
  static void log(const std::string& message, const std::string& color,
                  const int& replace) {
    if (replace) {
      std::cout << "\r" << std::setw(replace) << std::setfill(' ')
                << "\r";
    }

    std::cout << color << "[Arnelify POD]: " << message << "\033[0m";
    std::cout.flush();
  }

 public:
  static void primary(const std::string& message, const int& replace = 0) {
    Logger::log(message, "\033[0m", replace);
  }

  static void success(const std::string& message, const int& replace = 0) {
    Logger::log(message, "\033[32m", replace);
  }

  static void warning(const std::string& message, const int& replace = 0) {
    Logger::log(message, "\033[33m", replace);
  }

  static void danger(const std::string& message, const int& replace = 0) {
    Logger::log(message, "\033[31m", replace);
  }
};

#endif