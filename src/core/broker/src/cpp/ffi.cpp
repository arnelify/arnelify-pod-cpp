#ifndef ARNELIFY_BROKER_FFI_CPP
#define ARNELIFY_BROKER_FFI_CPP

#include "index.cpp"

ArnelifyBroker* broker = nullptr;

extern "C" {

void broker_create() {
  broker = new ArnelifyBroker();
}

void broker_destroy() {
  broker = nullptr;
}

const char* broker_get_datetime() {
  const std::string datetime = broker->getDateTime();
  char* cDateTime = new char[datetime.length() + 1];
  std::strcpy(cDateTime, datetime.c_str());
  return cDateTime;
}

const char* broker_get_uuid() {
  const std::string uuid = broker->getUuId();
  char* cUuId = new char[uuid.length() + 1];
  std::strcpy(cUuId, uuid.c_str());
  return cUuId;
}

const char* broker_serialize(const char* cDeserialized) {
  Json::Value deserialized;
  Json::CharReaderBuilder reader;
  std::string errors;
  std::istringstream iss(cDeserialized);
  if (!Json::parseFromStream(reader, iss, &deserialized, &errors)) {
    std::cout
        << "[ArnelifyBroker FFI]: C error: cDeserialized must be a valid JSON."
        << std::endl;
    exit(1);
  }

  const std::string serialized = Serializer::serialize(deserialized);
  char* cSerialized = new char[serialized.length() + 1];
  std::strcpy(cSerialized, serialized.c_str());
  return cSerialized;
}

const char* broker_deserialize(const char* cSerialized) {
  const Json::Value deserialized = Serializer::deserialize(cSerialized);
  Json::StreamWriterBuilder writer;
  writer["indentation"] = "";
  writer["emitUTF8"] = true;

  const std::string serialized = Json::writeString(writer, deserialized);
  char* cDeserialized = new char[serialized.length() + 1];
  std::strcpy(cDeserialized, serialized.c_str());
  return cDeserialized;
}

void broker_free(const char* cPointer) {
  if (cPointer) delete[] cPointer;
}
};

#endif