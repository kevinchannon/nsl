#pragma once

#include "udp_emitter.hpp"
#include "tftp_types.hpp"

#include <array>
#include <cstdint>
#include <istream>
#include <memory>
#include <string_view>
#include <utility>
#include <sstream>
#include <variant>

namespace raven::tftp {

class client {
 public:
  explicit client(std::unique_ptr<udp::emitter> udp_emitter) : _udp_emitter{std::move(udp_emitter)} {}

  void send(std::string_view filename, std::istream& data);

 private:
  [[nodiscard]] static std::stringstream _insert_write_request(std::stringstream&& os, std::string_view filename);

  std::unique_ptr<udp::emitter> _udp_emitter;
};

}  // namespace raven::tftp
