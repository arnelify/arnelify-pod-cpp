#ifndef ARNELIFY_TRANSMITTER_CALLBACK_HPP
#define ARNELIFY_TRANSMITTER_CALLBACK_HPP

#include <functional>

using ArnelifyTransmitterCallback =
    std::function<void(const std::string&, const bool&)>;

#endif