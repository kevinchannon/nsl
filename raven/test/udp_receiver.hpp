#pragma once

#include "framework.h"

#include "udp_types.hpp"

#include <boost/asio.hpp>

#include <cstdint>
#include <format>
#include <functional>
#include <istream>
#include <memory>
#include <optional>
#include <ostream>
#include <span>
#include <stdexcept>
#include <string>

namespace raven::test::udp {

  using callback_t = std::function<void(std::istream&, size_t)>;

class receiver {
 public:
  explicit receiver(boost::asio::io_context& io,
                         ::raven::udp::port_number port,
                         callback_t handle_data,
                         std::optional<int> recv_buf_size = std::nullopt)
      : _socket(io, boost::asio::ip::udp::endpoint{boost::asio::ip::udp::v4(), port})
      , _endpoint{_resolve_endpoint(io, "localhost", port)}
      , _handle_data{std::move(handle_data)} {
    if (recv_buf_size) {
      _socket.set_option(boost::asio::socket_base::receive_buffer_size{*recv_buf_size});
    }
    _receive();
  }

  ~receiver() { stop(); }

  void stop() { _socket.close(); }

  ::raven::udp::port_number connected_port() const { return _endpoint.port(); }

 private:
  [[nodiscard]] static boost::asio::ip::udp::endpoint _resolve_endpoint(boost::asio::io_context& io,
                                                                        std::string host,
                                                                        std::uint16_t port) {
    boost::asio::ip::udp::resolver resolver(io);
    boost::asio::ip::udp::resolver::query query(
        boost::asio::ip::udp::v4(), host, std::to_string(static_cast<std::uint32_t>(port)));
    return *resolver.resolve(query);
  }

  void _receive() {
    auto recv_buf = _recv_data.prepare(_recv_buf_size);
    _socket.async_receive_from(recv_buf, _endpoint, [this](auto&& err, auto&& n) {
      if (err) {
        return;
      }

      _recv_data.commit(n);

      auto data_stream = std::istream{&_recv_data};
      _handle_data(data_stream, n);

      _recv_data.consume(n);
      _receive();
    });
  }

  boost::asio::ip::udp::socket _socket;
  boost::asio::ip::udp::endpoint _endpoint{};
  callback_t _handle_data;
  static constexpr auto _recv_buf_size = 8192;
  boost::asio::streambuf _recv_data;
};
}  // namespace raven::test::udp
