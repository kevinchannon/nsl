#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <ostream>
#include <span>
#include <string>

namespace boost::asio {
class io_context;
}

namespace raven::udp {

struct receiver {
  virtual ~receiver() = default;

  using callback_t = std::function<void(size_t)>;

  static std::unique_ptr<receiver> create(boost::asio::io_context& io,
                                              std::uint16_t port,
                                              std::span<char> buffer,
                                              callback_t handle_data);

  virtual void stop() = 0;
};

}  // namespace raven
