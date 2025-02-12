#ifndef ARNELIFY_RECEIVER_OPTS_HPP
#define ARNELIFY_RECEIVER_OPTS_HPP

#include <filesystem>
#include <iostream>

struct ArnelifyReceiverOpts final {
  const bool RECEIVER_ALLOW_EMPTY_FILES;
  const std::string RECEIVER_CLIENT;
  const bool RECEIVER_KEEP_EXTENSIONS;
  const int RECEIVER_MAX_FIELDS;
  const std::size_t RECEIVER_MAX_FIELDS_SIZE_TOTAL_MB;
  const int RECEIVER_MAX_FILES;
  const std::size_t RECEIVER_MAX_FILES_SIZE_TOTAL_MB;
  const std::size_t RECEIVER_MAX_FILE_SIZE_MB;
  const std::filesystem::path RECEIVER_UPLOAD_DIR;

  ArnelifyReceiverOpts(const bool &a, const std::string &c, const bool &k,
                       const int &mfd, const std::size_t &mfdst, const int &mfl,
                       const std::size_t &mflst, const std::size_t &mfls,
                       const std::string &u = "./src/storage/upload")
      : RECEIVER_ALLOW_EMPTY_FILES(a),
        RECEIVER_CLIENT(c),
        RECEIVER_KEEP_EXTENSIONS(k),
        RECEIVER_MAX_FIELDS(mfd),
        RECEIVER_MAX_FIELDS_SIZE_TOTAL_MB(mfdst),
        RECEIVER_MAX_FILES(mfl),
        RECEIVER_MAX_FILES_SIZE_TOTAL_MB(mflst),
        RECEIVER_MAX_FILE_SIZE_MB(mfls),
        RECEIVER_UPLOAD_DIR(u) {};
};

#endif