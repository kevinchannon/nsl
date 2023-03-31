#pragma once

#include <wite/io/byte_stream.hpp>

#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>

namespace raven::tftp {

using byte         = char;
using packet_bytes = std::array<char, 516>;

class request_base {
 public:
  explicit request_base(std::uint16_t op_code) : op_code{op_code} {}
  const std::uint16_t op_code{};
};

class write_request : public request_base {
 public:
  enum class write_mode { octet, netascii };

  explicit write_request(std::string_view filename)
      : request_base{2}, filename{_validated_filename(filename)}, mode{write_mode::octet} {}

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

 public:
  request(write_request wr) : _base_t{std::move(wr)} {}
};

}  // namespace raven::tftp
