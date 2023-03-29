#pragma once

#include <array>
#include <cstdint>
#include <istream>
#include <string_view>

namespace raven::tftp {

using byte = char;
using packet_bytes = std::array<byte, 516>;

[[nodiscard]] packet_bytes make_write_request_bytes(std::string_view filename);

}  // namespace raven::tftp
