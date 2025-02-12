#ifndef ARNELIFY_SERVER_OPTS_HPP
#define ARNELIFY_SERVER_OPTS_HPP

#include <filesystem>
#include <iostream>

struct ArnelifyServerOpts final {
  const bool SERVER_ALLOW_EMPTY_FILES;
  const std::size_t SERVER_BLOCK_SIZE_KB;
  const std::string SERVER_CHARSET;
  const bool SERVER_GZIP;
  const bool SERVER_KEEP_EXTENSIONS;
  const int SERVER_MAX_FIELDS;
  const std::size_t SERVER_MAX_FIELDS_SIZE_TOTAL_MB;
  const int SERVER_MAX_FILES;
  const std::size_t SERVER_MAX_FILES_SIZE_TOTAL_MB;
  const std::size_t SERVER_MAX_FILE_SIZE_MB;
  const int SERVER_PORT;
  const int SERVER_QUEUE_LIMIT;
  const std::filesystem::path SERVER_UPLOAD_DIR;

  ArnelifyServerOpts(const bool &a, const int &b, const std::string &c,
                     const bool &g, const bool &k, const int &mfd,
                     const int &mfdst, const int &mfl, const int &mflst,
                     const int &mfls, const int &p, const int &q,
                     const std::string &u = "./src/storage/upload")
      : SERVER_ALLOW_EMPTY_FILES(a),
        SERVER_BLOCK_SIZE_KB(b),
        SERVER_CHARSET(c),
        SERVER_GZIP(g),
        SERVER_KEEP_EXTENSIONS(k),
        SERVER_MAX_FIELDS(mfd),
        SERVER_MAX_FIELDS_SIZE_TOTAL_MB(mfdst),
        SERVER_MAX_FILES(mfl),
        SERVER_MAX_FILES_SIZE_TOTAL_MB(mflst),
        SERVER_MAX_FILE_SIZE_MB(mfls),
        SERVER_PORT(p),
        SERVER_QUEUE_LIMIT(q),
        SERVER_UPLOAD_DIR(u) {};
};

#endif