#include "udp_input_socket.hpp"

#include "framework.h"
#include <boost/asio.hpp>

#include <format>
#include <span>
#include <stdexcept>

using boost::asio::ip::udp;

namespace raven {

namespace detail {

  class udp_input_socket_impl : public udp_input_socket {
   public:
    explicit udp_input_socket_impl(boost::asio::io_context& io,
                                   std::uint16_t port,
                                   std::span<char> buffer,
                                   callback_t handle_data)
        : _socket(io, udp::endpoint(udp::v4(), port))
        , _endpoint{_resolve_endpoint(io, "127.0.0.1", port)}
        , _buffer{std::move(buffer)}
        , _handle_data{std::move(handle_data)} {
      _receive();
    }

    void stop() override { _socket.close(); }

    ~udp_input_socket_impl() { stop(); }

   private:
    [[nodiscard]] static udp::endpoint _resolve_endpoint(boost::asio::io_context& io, std::string host, std::uint16_t port) {
      udp::resolver resolver(io);
      udp::resolver::query query(udp::v4(), host, std::to_string(static_cast<std::uint32_t>(port)));
      return *resolver.resolve(query);
    }

    void _receive() {
      std::ranges::fill(_buffer, '\0');
      _socket.async_receive_from(boost::asio::buffer(_buffer), _endpoint, [this](auto&& err, auto&& bytes_received) {
        if (!err) {
          _handle_data(bytes_received);
          _receive();
        }
      });
    }

    udp::socket _socket;
    udp::endpoint _endpoint{};
    std::span<char> _buffer;
    callback_t _handle_data;
  };

}  // namespace detail

udp_input_socket::ptr udp_input_socket::create(boost::asio::io_context& io,
                                               std::uint16_t port,
                                               std::span<char> buffer,
                                               callback_t handle_data) {
  return std::make_unique<detail::udp_input_socket_impl>(io, port, std::move(buffer), std::move(handle_data));
}

}  // namespace raven
