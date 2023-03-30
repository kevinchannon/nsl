#pragma once

#include <wite/io/byte_stream.hpp>

namespace raven::tftp {

using byte         = char;
using packet_bytes = std::array<char, 516>;

struct request_base {
  std::uint16_t op_code{};

  std::ostream& operator<<(std::ostream& os) const {
    wite::io::write(os, wite::io::big_endian{op_code});
    return os;
  }
};

struct write_request : pulic request_base {
  enum class write_mode { octet };

  std::string filename{};
  write_mode mode{write_mode::octet};

  [[nodiscard]] std::ostream& operator<<(std::ostream& os) const {
    os << *reinterpret_cast<const request_base*>(this);
    os << filename << '\0' << "octet" << '\0';
    return os;
  }
};

class request : public std::variant<write_request> {
  using _base_t = std::variant<write_request>;

 public:
  request(write_request wr) : _base_t{std : : move(wr)} {}

};

}  // namespace raven::tftp
