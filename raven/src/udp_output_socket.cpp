#include "udp_output_socket.hpp"

#include "framework.h"

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>

#include <algorithm>
#include <format>
#include <stdexcept>
#include <vector>

using boost::asio::ip::udp;

namespace raven {

namespace detail {

  class udp_output_socket_impl : public udp_output_socket {
   public:
    explicit udp_output_socket_impl(boost::asio::io_context& io, std::string host, std::uint16_t port)
        : _io{io}, _socket{_io, udp::endpoint(udp::v4(), 0)}, _endpoint{} {
      _endpoint = _resolve_endpoint(_io, std::move(host), port);
    }

    ~udp_output_socket_impl() { _socket.close(); }

    [[nodiscard]] size_t send(std::istream& data) override { 
      auto buf = std::array<char, 1024>{};
      auto sent_byte_count = size_t{0};
      do {
        data.read(buf.data(), static_cast<std::streamsize>(buf.size()));
        const auto read_bytes = data.gcount();
        if (read_bytes <= 0) {
          break;
        }

        sent_byte_count += _socket.send_to(boost::asio::buffer(buf, data.gcount()), _endpoint);
      } while (true);

      return sent_byte_count;
    }

   private:
    [[nodiscard]] static udp::endpoint _resolve_endpoint(boost::asio::io_context& io, std::string host, std::uint16_t port) {
      udp::resolver resolver(io);
      udp::resolver::query query(udp::v4(), host, std::to_string(static_cast<std::uint32_t>(port)));
      return *resolver.resolve(query);
    }

    boost::asio::io_context& _io;
    udp::socket _socket;
    udp::endpoint _endpoint;
  };

}  // namespace detail

udp_output_socket::ptr udp_output_socket::create(boost::asio::io_context& io, std::string ip, std::uint16_t port) {
  return std::make_unique<detail::udp_output_socket_impl>(io, std::move(ip), port);
}

}  // namespace raven
