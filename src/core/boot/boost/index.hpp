#include <iostream>
#include <utility>
#include <variant>

#include "boost/asio.hpp"
#include "boost/thread.hpp"

#include "contracts/acceptor.h"
#include "contracts/callback.h"
#include "contracts/endpoint.h"
#include "contracts/exception.h"
#include "contracts/ioc.h"

#include "env/index.hpp"
#include "boot/boost/session/index.hpp"
#include "boot/boost/uploader/index.hpp"

class BoostServer {
 private:
  int threads;
  bool isRunning = true;
  BoostIOC ioc;
  BoostEndpoint endpoint;
  BoostAcceptor acceptor;
  BoostHandler handler;

  const std::string trim(const std::string& str) {
    const auto start =
        std::find_if(str.begin(), str.end(),
                     [](unsigned char c) { return !std::isspace(c); });
    const auto end =
        std::find_if(str.rbegin(), str.rend(), [](unsigned char c) {
          return !std::isspace(c);
        }).base();
    return (start < end) ? std::string(start, end) : std::string();
  }

  const int parseInt(const std::variant<int, std::string>& raw) {
    return std::visit(
        [this](auto&& value) -> int {
          if constexpr (std::is_same_v<std::decay_t<decltype(value)>, int>) {
            return value;
          } else {
            const std::string newValue = this->trim(value);

            try {
              return std::stoi(newValue);

            } catch (const std::invalid_argument&) {
              throw std::runtime_error("Invalid number: " + value);

            } catch (const std::out_of_range&) {
              throw std::runtime_error("Number out of range: " + value);
            }
          }
        },

        raw);
  }

  BoostMiddleware middleware = [this](const BoostRawReq& rawReq,
                                      BoostNext next) {
        BoostRawRes rawRes(Json::objectValue);
    const bool isPost = rawReq.method() == boost::beast::http::verb::post;
    const bool isPut = rawReq.method() == boost::beast::http::verb::put;
    const bool isPatch = rawReq.method() == boost::beast::http::verb::patch;
    const bool isDelete = rawReq.method() == boost::beast::http::verb::delete_;

    const bool hasBody = isPost || isPut || isPatch || isDelete;
    if (hasBody) {
      const bool keepExtensions = env.UPLOADER_KEEP_EXTENSIONS == "true";
      const int maxFields = this->parseInt(env.UPLOADER_MAX_FIELDS);
      const int maxFieldsSizeTotalMb =
          this->parseInt(env.UPLOADER_MAX_FIELDS_SIZE_TOTAL_MB);
      const bool allowEmptyFiles = env.UPLOADER_ALLOW_EMPTY_FILES == "true";
      const int maxFiles = this->parseInt(env.UPLOADER_MAX_FILES);
      const int maxFilesSizeTotalMb =
          this->parseInt(env.UPLOADER_MAX_FILES_SIZE_TOTAL_MB);
      const int maxFileSizeMb = this->parseInt(env.UPLOADER_MAX_FILE_SIZE_MB);

      Uploader uploader(keepExtensions, maxFields, maxFieldsSizeTotalMb,
                        allowEmptyFiles, maxFiles, maxFilesSizeTotalMb,
                        maxFileSizeMb);

      rawRes = uploader.parse(rawReq);
      return next(rawRes);

    } else {
      rawRes["code"] = 200;
      rawRes["success"] = Json::Value(Json::objectValue);
      return next(rawRes);
    }
  };

  void accept() {
    this->acceptor.async_accept([this](BoostError error, BoostSocket socket) {
      if (!this->isRunning) return;
      if (error) {
        std::cout << error << std::endl;
        return;
      }

      const int bodyLimitMb =
          this->parseInt(env.UPLOADER_MAX_FILES_SIZE_TOTAL_MB);
      auto session = std::make_shared<Session>(
          bodyLimitMb, this->handler, this->middleware, std::move(socket));
      session->run();
      accept();
    });
  }

 public:
  BoostServer(std::variant<int, std::string> p)
      : threads(std::thread::hardware_concurrency()),
        ioc{threads},
        endpoint(boost::asio::ip::tcp::v4(), this->parseInt(p)),
        acceptor(ioc, endpoint) {
    this->acceptor.listen();
    this->accept();
  }

  void setHandler(BoostHandler handler) { this->handler = handler; }

  void start(BoostCallback callback = []() {}) {
    try {
      boost::thread thread([this]() { this->ioc.run(); });

      this->isRunning = true;

      callback();

      thread.join();

    } catch (BoostException& e) {
      std::cerr << "Error: " << e.what() << std::endl;
    }
  }

  void stop(BoostCallback callback = []() {}) {
    this->isRunning = false;
    this->acceptor.close();
    this->ioc.stop();
    callback();
  }
};