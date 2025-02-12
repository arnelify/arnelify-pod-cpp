#ifndef ARNELIFY_BROKER_SERIALIZER_HPP
#define ARNELIFY_BROKER_SERIALIZER_HPP

#include "json.h"

class Serializer {
 public:
  static const Json::Value deserialize(const std::string& serialized) {
    Json::Value deserialized;
    Json::CharReaderBuilder reader;
    std::string errors;

    std::istringstream stream(serialized);
    if (!Json::parseFromStream(reader, stream, &deserialized, &errors)) {
      throw std::runtime_error("Failed to parse JSON: " + errors);
    }

    return deserialized;
  }

  static const std::string serialize(const Json::Value& deserialized) {
    Json::StreamWriterBuilder writer;
    std::string serialized = Json::writeString(writer, deserialized);
    return serialized;
  }
};

#endif