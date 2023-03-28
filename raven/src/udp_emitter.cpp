#include "udp_emitter.hpp"

#include "framework.h"

#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/asio/io_context.hpp>

#include <algorithm>
#include <format>
#include <stdexcept>
#include <vector>

namespace bai = boost::asio::ip;

namespace raven::udp {

namespace detail {

  class emitter_impl : public emitter {
   public:
    explicit emitter_impl(std::string host, std::uint16_t port)
        : _io{}, _socket{_io, bai::udp::endpoint(bai::udp::v4(), 0)}, _endpoint{} {
      _endpoint = _resolve_endpoint(_io, std::move(host), port);
    }

    ~emitter_impl() { _socket.close(); }

    [[nodiscard]] size_t send(std::istream& data) override { 
      auto sent_byte_count = size_t{0};
      do {
        data.read(_send_buffer.data(), static_cast<std::streamsize>(_send_buffer.size()));
        const auto read_bytes = data.gcount();
        if (read_bytes <= 0) {
          break;
        }

        sent_byte_count += _socket.send_to(boost::asio::buffer(_send_buffer, data.gcount()), _endpoint);
      } while (true);

      return sent_byte_count;
    }

   private:
    [[nodiscard]] static bai::udp::endpoint _resolve_endpoint(boost::asio::io_context& io, std::string host, std::uint16_t port) {
      bai::udp::resolver resolver(io);
      bai::udp::resolver::query query(bai::udp::v4(), host, std::to_string(static_cast<std::uint32_t>(port)));
      return *resolver.resolve(query);
    }

    boost::asio::io_context _io;
    bai::udp::socket _socket;
    bai::udp::endpoint _endpoint;
    std::array<char, 1024> _send_buffer;
  };

}  // namespace detail

std::unique_ptr<emitter> emitter::create(std::string ip, std::uint16_t port) {
  return std::make_unique<detail::emitter_impl>(std::move(ip), port);
}

}  // namespace ravent
