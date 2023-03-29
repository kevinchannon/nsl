#include "udp_receiver.hpp"

#include "framework.h"
#include <boost/asio.hpp>

#include <format>
#include <span>
#include <stdexcept>

namespace bai = boost::asio::ip;

namespace raven::udp {

namespace detail {

  class receiver_impl : public receiver {
   public:
    explicit receiver_impl(boost::asio::io_context& io,
                                   std::uint16_t port,
                                   std::span<char> buffer,
                                   callback_t handle_data)
        : _socket(io, bai::udp::endpoint{bai::udp::v4(), port})
        , _endpoint{_resolve_endpoint(io, "localhost", port)}
        , _buffer{std::move(buffer)}
        , _handle_data{std::move(handle_data)} {
      _receive();
    }

    void stop() override { _socket.close(); }

    ~receiver_impl() { stop(); }

   private:
    [[nodiscard]] static bai::udp::endpoint _resolve_endpoint(boost::asio::io_context& io, std::string host, std::uint16_t port) {
      bai::udp::resolver resolver(io);
      bai::udp::resolver::query query(bai::udp::v4(), host, std::to_string(static_cast<std::uint32_t>(port)));
      return *resolver.resolve(query);
    }

    void _receive() {
      std::ranges::fill(_buffer, '\0');
      _socket.async_receive_from(boost::asio::buffer(_buffer), _endpoint, [this](auto&& err, auto&& bytes_received) {
        if (err) {
          return;
        }
          
        _handle_data(bytes_received);
        _receive();
      });
    }

    bai::udp::socket _socket;
    bai::udp::endpoint _endpoint{};
    std::span<char> _buffer;
    callback_t _handle_data;
  };

}  // namespace detail

std::unique_ptr<receiver> receiver::create(boost::asio::io_context& io,
                                               std::uint16_t port,
                                               std::span<char> buffer,
                                               callback_t handle_data) {
  return std::make_unique<detail::receiver_impl>(io, port, std::move(buffer), std::move(handle_data));
}

}  // namespace raven
