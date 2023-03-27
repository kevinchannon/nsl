#pragma once

#include <cstdint>
#include <ostream>
#include <memory>
#include <string>
#include <functional>
#include <span>

namespace boost::asio {
class io_context;
}

namespace raven {

struct udp_input_socket {
  using ptr = std::unique_ptr<udp_input_socket>;
  using callback_t = std::function<void(size_t)>;

  static ptr create(boost::asio::io_context& io, std::uint16_t port, std::span<char> buffer, callback_t handle_data);

  virtual void stop() = 0;

  virtual ~udp_input_socket() = default;
};

}  // namespace raven
