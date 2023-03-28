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
    explicit emitter_impl(boost::asio::io_context& io, std::string host, std::uint16_t port)
        : _io{io}, _socket{_io, bai::udp::endpoint(bai::udp::v4(), 0)}, _endpoint{} {
      _endpoint = _resolve_endpoint(_io, std::move(host), port);
    }

    ~emitter_impl() { _socket.close(); }

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
    [[nodiscard]] static bai::udp::endpoint _resolve_endpoint(boost::asio::io_context& io, std::string host, std::uint16_t port) {
      bai::udp::resolver resolver(io);
      bai::udp::resolver::query query(bai::udp::v4(), host, std::to_string(static_cast<std::uint32_t>(port)));
      return *resolver.resolve(query);
    }

    boost::asio::io_context& _io;
    bai::udp::socket _socket;
    bai::udp::endpoint _endpoint;
  };

}  // namespace detail

std::unique_ptr<emitter> emitter::create(boost::asio::io_context& io, std::string ip, std::uint16_t port) {
  return std::make_unique<detail::emitter_impl>(io, std::move(ip), port);
}

}  // namespace raven
