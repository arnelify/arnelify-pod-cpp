#ifndef ARNELIFY_UNIX_DOMAIN_SOCKET_CLIENT_OPTS_HPP
#define ARNELIFY_UNIX_DOMAIN_SOCKET_CLIENT_OPTS_HPP

#include <filesystem>
#include <iostream>

struct ArnelifyUnixDomainSocketClientOpts final {
  const std::size_t UDS_BLOCK_SIZE_KB;
  const std::filesystem::path UDS_SOCKET_PATH;

  ArnelifyUnixDomainSocketClientOpts(
      const int &b, const std::string &s = "/tmp/arnelify.sock")
      : UDS_BLOCK_SIZE_KB(b), UDS_SOCKET_PATH(s) {};
};

#endif