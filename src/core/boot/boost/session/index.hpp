#ifndef BOOST_SESSION_HPP
#define BOOST_SESSION_HPP

#include <iostream>
#include <variant>
#include <optional>

#include "boost/asio.hpp"
#include "boost/beast.hpp"
#include "boost/thread.hpp"
#include "json.h"

#include "contracts/buffer.h"
#include "contracts/error.h"
#include "contracts/handler.h"
#include "contracts/parser.h"
#include "contracts/socket.h"

class Session : public std::enable_shared_from_this<Session> {
 private:
  int bodyLimitMb;
  BoostBuffer buffer;
  BoostHandler handler;
  BoostMiddleware middleware;
  BoostParser parser;
  BoostRes response;
  BoostSocket socket;

  const std::string getContentType(const BoostRawReq& rawReq,
                                   auto& contentTypeKey) {
    const std::string rawContentType = rawReq[contentTypeKey].to_string();
    const bool isMultipart = rawContentType.starts_with("multipart/form-data");
    if (isMultipart) return rawContentType.substr(0, 19);
    return rawContentType;
  }

  const Json::Value getCookie(const BoostRawReq& rawReq) {
    Json::Value cookie = Json::objectValue;
    const auto it = rawReq.find(boost::beast::http::field::cookie);

    const std::size_t maxCookieSizeTotalKb = 4096;
    const std::size_t maxCookieSizeTotal = maxCookieSizeTotalKb * 1024;
    if (it != rawReq.end()) {
      const std::string cookies = it->value().to_string();
      if (cookies.size() > maxCookieSizeTotal) return cookie;
      std::istringstream stream(cookies);
      std::string pair;

      while (std::getline(stream, pair, ';')) {
        const std::size_t pos = pair.find('=');
        if (pos != std::string::npos) {
          const bool hasSpace = pair.front() == ' ';
          const int nameStart = hasSpace ? 1 : 0;

          const std::string name = pair.substr(nameStart, pos - 1);
          const std::string value = pair.substr(pos + 1);
          cookie[name] = value;
        }
      }
    }

    return cookie;
  }

  const std::string getMethod(const BoostRawReq& rawReq) {
    if (rawReq.method() == boost::beast::http::verb::connect) return "CONNECT";
    if (rawReq.method() == boost::beast::http::verb::delete_) return "DELETE";
    if (rawReq.method() == boost::beast::http::verb::head) return "HEAD";
    if (rawReq.method() == boost::beast::http::verb::options) return "OPTIONS";
    if (rawReq.method() == boost::beast::http::verb::patch) return "PATCH";
    if (rawReq.method() == boost::beast::http::verb::post) return "POST";
    if (rawReq.method() == boost::beast::http::verb::put) return "PUT";
    if (rawReq.method() == boost::beast::http::verb::trace) return "TRACE";
    return "GET";
  }

  const Json::Value getParams(const std::string& url) {
    Json::Value params = Json::objectValue;
    const std::size_t queryPosStart = url.find('?');
    const bool hasParams = queryPosStart != std::string::npos;
    if (!hasParams) return params;

    const std::string queryString = url.substr(queryPosStart + 1);

    std::string param;
    std::istringstream queryStream(queryString);

    while (std::getline(queryStream, param, '&')) {
      const std::size_t equalPosStart = param.find('=');
      const bool hasEqual = equalPosStart != std::string::npos;
      if (hasEqual) {
        const std::string key = param.substr(0, equalPosStart);
        const std::string value = param.substr(equalPosStart + 1);
        params[key] = value;
      }
    }

    return params;
  }

  void write() {
    auto self = shared_from_this();

    this->response.prepare_payload();

    boost::beast::http::async_write(
        this->socket, this->response,
        [this, self](BoostError ec, std::size_t bytes_transferred) {
          boost::ignore_unused(bytes_transferred);
          this->socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send,
                                ec);
        });
  }

  void read() {
    auto self = shared_from_this();
    const std::size_t maxFilesSizeTotal = this->bodyLimitMb * 1024 * 1024;
    this->parser.body_limit(maxFilesSizeTotal);

    boost::beast::http::async_read(
        this->socket, this->buffer, this->parser,
        [this, self](BoostError ec, std::size_t bytesTransferred) {
          boost::ignore_unused(bytesTransferred);
          Json::Value res = Json::objectValue;

          const auto& contentTypeKey = boost::beast::http::field::content_type;
          const auto& serverKey = boost::beast::http::field::server;

          this->response.version(11);
          this->response.set(serverKey, "Arnelify POD");
          Json::StreamWriterBuilder writer;
          writer["indentation"] = "";

          if (ec) {
            res["code"] = 409;
            res["error"] = ec.message();

            this->response.result(res["code"].asInt());
            this->response.set(contentTypeKey, "application/json");
            const std::string json = Json::writeString(writer, res);
            this->response.body() = json;
            this->write();
            return;
          }

          const std::optional<BoostRawReq> rawReqOpt = this->parser.release();
          const BoostRawReq rawReq = *rawReqOpt;

          const std::string scheme = "http://";
          const std::string host = rawReq["Host"].to_string();
          const std::string query = rawReq.target().to_string();
          const std::string url = scheme + host + query;

          const bool isUrlLen = url.length() > 2048;
          if (isUrlLen) {
            res["code"] = 414;
            res["error"] = "Request-URI too long";

            this->response.result(res["code"].asInt());
            this->response.set(contentTypeKey, "application/json");
            const std::string json = Json::writeString(writer, res);
            this->response.body() = json;
            this->write();
            return;
          }

          this->middleware(rawReq, [this, &host, &contentTypeKey, &writer,
                                    &rawReq, &res](BoostRawRes& rawRes) {
            BoostReq req = Json::objectValue;
            std::string target = rawReq.target().to_string();
            std::size_t pathEnd = target.find('?');
            
            if (rawRes["code"].asInt() != 200) {
              res["code"] = rawRes["code"];
              res["error"] = rawRes["error"];

              this->response.result(res["code"].asInt());
              this->response.set(contentTypeKey, "application/json");
              const std::string json = Json::writeString(writer, res);
              this->response.body() = json;
              this->write();
              return;
            }

            req["cookie"] = this->getCookie(rawReq);
            req["body"] = rawRes["success"];
            req["clientIP"] =
                this->socket.remote_endpoint().address().to_string();
            req["clientPort"] = this->socket.remote_endpoint().port();
            req["contentType"] = this->getContentType(rawReq, contentTypeKey);
            req["host"] = host;
            req["localIP"] =
                this->socket.local_endpoint().address().to_string();
            req["localPort"] = this->socket.local_endpoint().port();
            req["method"] = this->getMethod(rawReq);
            req["target"] = target.substr(0, pathEnd);
            req["params"] = this->getParams(target);
            req["user_agent"] = rawReq["User-Agent"].to_string();

            this->handler(req, this->response);
            this->write();
          });
        });
  }

 public:
  Session(int b, BoostHandler h, BoostMiddleware m, BoostSocket s)
      : bodyLimitMb(std::move(b)),
        handler(std::move(h)),
        middleware(std::move(m)),
        socket(std::move(s)) {}

  void run() {
    std::thread thread([this]() { this->read(); });
    thread.join();
  }
};

#endif