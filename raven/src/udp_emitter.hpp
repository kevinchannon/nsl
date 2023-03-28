#pragma once

#include <memory>
#include <string>
#include <cstdint>
#include <istream>

namespace boost::asio {
class io_context;
}

namespace raven::udp {

struct emitter {
  static std::unique_ptr<emitter> create(boost::asio::io_context& io, std::string ip, std::uint16_t port);

  virtual ~emitter() = default;

  [[nodiscard]] virtual size_t send(std::istream& data) = 0;
};

}  // namespace raven
