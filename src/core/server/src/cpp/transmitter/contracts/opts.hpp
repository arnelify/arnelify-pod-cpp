#ifndef ARNELIFY_TRANSMITTER_OPTS_HPP
#define ARNELIFY_TRANSMITTER_OPTS_HPP

#include <iostream>

struct ArnelifyTransmitterOpts final {
  const std::size_t TRANSMITTER_BLOCK_SIZE_KB;
  const std::string TRANSMITTER_CHARSET;
  const bool TRANSMITTER_GZIP;
  const std::string TRANSMITTER_CLIENT;

  ArnelifyTransmitterOpts(const std::size_t &bs, const std::string &ch,
                          const bool &g, const std::string &cl)
      : TRANSMITTER_BLOCK_SIZE_KB(bs),
        TRANSMITTER_CHARSET(ch),
        TRANSMITTER_GZIP(g),
        TRANSMITTER_CLIENT(cl) {};
};

#endif