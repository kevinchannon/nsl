#pragma once

#include <memory>
#include <string>
#include <cstdint>
#include <istream>

namespace raven {

struct udp_output_socket {
  using ptr = std::unique_ptr<udp_output_socket>;

  static ptr create(std::string ip, std::uint16_t port);

  virtual ~udp_output_socket() = default;

  [[nodiscard]] virtual int send(std::istream& data) = 0;
};

}  // namespace raven
