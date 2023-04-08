#pragma once

#include "framework.h"

#include "types.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream.hpp>

#include <chrono>
#include <cstdint>
#include <istream>
#include <memory>
#include <span>
#include <string>
#include <thread>

namespace boost::asio {
class io_context;
}

namespace raven::udp {

namespace detail {
  class source {
    struct kernel {
      kernel(boost::asio::io_context& io, boost::asio::ip::udp::endpoint endpoint) : io{io}, socket{io, endpoint} {}

      ~kernel() { socket.close(); }

      boost::asio::io_context& io;
      boost::asio::ip::udp::socket socket;
    };

   public:
    using char_type = char;
    using category  = boost::iostreams::source_tag;

    explicit source(boost::asio::io_context& io, port_number port)
        : _kernel{std::make_shared<kernel>(io, _resolve_endpoint(io, "0.0.0.0", port))} {}

    source()                         = delete;
    source(const source&)            = default;
    source& operator=(const source&) = delete;
    source(source&&)                 = default;
    source& operator=(source&&)      = default;

    ~source() {}

    [[nodiscard]] std::streamsize read(char* s, std::streamsize n) {
      auto bytes_recvd = _kernel->socket.receive(boost::asio::buffer(s, n));

      return bytes_recvd;
    }

   private:
    [[nodiscard]] static boost::asio::ip::udp::endpoint _resolve_endpoint(boost::asio::io_context& io,
                                                                          std::string host,
                                                                          port_number port) {
      boost::asio::ip::udp::resolver resolver(io);
      boost::asio::ip::udp::resolver::query query(
          boost::asio::ip::udp::v4(), host, std::to_string(static_cast<std::uint32_t>(port)));
      return *resolver.resolve(query);
    }

    std::shared_ptr<kernel> _kernel;
  };
}  // namespace detail

using istreambuf = boost::iostreams::stream_buffer<detail::source>;
using istream    = boost::iostreams::stream<detail::source>;

}  // namespace raven::udp
