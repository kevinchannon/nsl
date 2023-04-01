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
  void connect(std::ostream& target, std::string_view filename);
};

}  // namespace raven::tftp
