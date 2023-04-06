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
      kernel(boost::asio::io_context& io, port_number port)
          : io{io}, socket{io, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 0)}, endpoint{} {
        endpoint = sink::_resolve_endpoint(io, "localhost", port);
      }

      ~kernel() { socket.close(); }

      boost::asio::io_context io;
      boost::asio::ip::udp::socket socket;
      boost::asio::ip::udp::endpoint endpoint;
    };

   public:
    using char_type = char;
    using category  = boost::iostreams::sink_tag;

    explicit source(boost::asio::io_context& io, port_number port) : _kernel{std::make_shared<kernel>(io, port)} {}

    source()                         = delete;
    source(const source&)            = default;
    source& operator=(const source&) = delete;
    source(source&&)                 = default;
    source& operator=(source&&)      = default;

    ~source() {}

    [[nodiscard]] std::streamsize read(const char* s, std::streamsize n) {
      return _kernel->socket.receive_from(boost::asio::buffer(s, n), _kernel->endpoint);
    }

   private:
    friend struct kernel;
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
