#pragma once

#include "framework.h"

#include "tftp_types.hpp"
#include "udp_emitter.hpp"
#include "udp_receiver.hpp"

#include <boost/asio/io_context.hpp>

#include <array>
#include <cstdint>
#include <istream>
#include <memory>
#include <sstream>
#include <string_view>
#include <utility>
#include <variant>
#include <thread>

namespace raven::tftp {

template <typename UdpRevc_T>
class client {
 public:
  explicit client(boost::asio::io_context& io) : _io{io} {}

  client()                         = delete;
  client(const client&)            = delete;
  client& operator=(const client&) = delete;
  client(client&&) noexcept        = default;
  client& operator=(client&&)      = delete;

  [[nodiscard]] connect(std::ostream& target, std::string_view filename) {
    auto lock     = std::unique_lock<std::mutex>{_mtx};
    auto udp_recv = UdpRecv_T{_io, udp::any_port, [this](std::istream& data, size_t n) { _receive_ack(data, n); }};
    
    target << write_request{filename};
    _data_receive.wait(lock);

    return udp_recv.connected_port();
  }

 private:

   void _receive_ack(std::istream& data, size_t size) {
    {
      auto lock = std::unique_lock<std::mutex>{_mtx};
      std::copy_n(std::istreambuf_iterator<char>{data}, size, _recv_bytes.begin());
    }

    _data_received.notify_all();
   }

    boost::asio::io)context& _io;
    std::array<char, 514> _recv_bytes;
    mutable std::mutex _mtx;
    std::condition _data_received;
};

}  // namespace raven::tftp
