#pragma once

#include <cstdint>
#include <ostream>
#include <memory>
#include <string>

namespace raven {

struct udp_input_socket {
  using ptr = std::unique_ptr<udp_input_socket>;

  static ptr create(std::uint16_t port);

  virtual ~udp_input_socket() = default;

  [[nodiscard]] virtual int recv(std::ostream& out) = 0;
};

}  // namespace raven
