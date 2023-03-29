#pragma once

#include <array>
#include <cstdint>
#include <istream>
#include <string_view>

namespace raven::tftp {

using byte = char;
using packet = std::array<byte, 516>;

[[nodiscard]] packet make_write_request(std::string_view filename);

}  // namespace raven::tftp
