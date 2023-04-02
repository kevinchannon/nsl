#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <ostream>
#include <span>
#include <string>
#include <istream>
#include <optional>

namespace boost::asio {
class io_context;
}

namespace raven::udp {

struct receiver {
  virtual ~receiver() = default;

  using callback_t = std::function<void(std::istream&, size_t)>;

  static std::unique_ptr<receiver> create(boost::asio::io_context& io, std::uint16_t port, callback_t handle_data, std::optional<int> recv_buf_size=std::nullopt);

  virtual void stop() = 0;
};

}  // namespace raven
