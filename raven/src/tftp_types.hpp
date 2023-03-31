#pragma once

#include <wite/io/byte_stream.hpp>
#include <wite/io/concepts.hpp>

#include <format>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>

namespace raven::tftp {

using byte         = char;
using packet_bytes = std::array<char, 516>;

class request_base {
 public:
  enum class request_type : std::uint16_t { write = 2 };

  explicit request_base(request_type type) : op_code{type} {}
  const request_type op_code{};
};

class write_request : public request_base {
 public:
  enum class write_mode { octet, netascii };

  explicit write_request(std::string_view filename)
      : request_base{request_type::write}, filename{_validated_filename(filename)}, mode{write_mode::octet} {}

  const std::string filename;
  const write_mode mode;

 private:
  [[nodiscard]] static std::string _validated_filename(std::string_view filename) {
    if (filename.length() > 503)
      throw std::invalid_argument{"filename exceeds 503 charaters"};

    return std::string{filename};
  }
};

class request : public std::variant<write_request> {
  using _base_t = std::variant<write_request>;

  friend std::ostream& operator<<(std::ostream& os, const write_request& req);

 public:
  request(write_request wr) : _base_t{std::move(wr)} {}
};

}  // namespace raven::tftp

namespace raven {

inline std::string_view to_string(tftp::write_request::write_mode mode) {
  using namespace tftp;

  switch (mode) {
    case write_request::write_mode::octet:
      return "octet";
    case write_request::write_mode::netascii:
      return "netascii";
    default:;
  }

  throw std::invalid_argument{std::format("Invalid write request mode: {}", static_cast<std::uint32_t>(mode))};
}

namespace tftp {

  inline std::ostream& operator<<(std::ostream& os, const write_request& req) {
    wite::io::write(os, static_cast<std::uint16_t>(req.op_code));
    os << req.filename << '\0' << to_string(req.mode) << '\0';

    return os;
  }
}  // namespace tftp
}  // namespace raven
