#pragma once

#include <memory>
#include <string>
#include <cstdint>
#include <istream>

namespace boost::asio {
class io_context;
}

namespace raven {

struct udp_output_socket {
  using ptr = std::unique_ptr<udp_output_socket>;

  static ptr create(boost::asio::io_context& io, std::string ip, std::uint16_t port);

  virtual ~udp_output_socket() = default;

  [[nodiscard]] virtual size_t send(std::istream& data) = 0;
};

}  // namespace raven
