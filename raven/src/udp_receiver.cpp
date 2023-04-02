#include "udp_receiver.hpp"

#include <boost/asio.hpp>
#include "framework.h"

#include <format>
#include <optional>
#include <span>
#include <stdexcept>

namespace bai = boost::asio::ip;

namespace raven::udp {

namespace detail {

  class receiver_impl : public receiver {
   public:
    explicit receiver_impl(boost::asio::io_context& io,
                           std::uint16_t port,
                           callback_t handle_data,
                           std::optional<int> recv_buf_size)
        : _socket(io, bai::udp::endpoint{bai::udp::v4(), port})
        , _endpoint{_resolve_endpoint(io, "localhost", port)}
        , _handle_data{std::move(handle_data)} {
      if (recv_buf_size) {
        _socket.set_option(boost::asio::socket_base::receive_buffer_size{*recv_buf_size});
      }
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

    bai::udp::socket _socket;
    bai::udp::endpoint _endpoint{};
    callback_t _handle_data;
    static constexpr auto _recv_buf_size = 8192;
    boost::asio::streambuf _recv_data;
  };

}  // namespace detail

std::unique_ptr<receiver> receiver::create(boost::asio::io_context& io,
                                           std::uint16_t port,
                                           callback_t handle_data,
                                           std::optional<int> recv_buf_size) {
  return std::make_unique<detail::receiver_impl>(io, port, std::move(handle_data), recv_buf_size);
}

}  // namespace raven::udp
