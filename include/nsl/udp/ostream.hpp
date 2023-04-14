#pragma once

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

namespace nsl::udp {

namespace detail {

  class sink {
    struct kernel {
      kernel(std::string host, port_number port)
          : io{}, socket{io, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 0)}, endpoint{} {
        endpoint = sink::_resolve_endpoint(io, std::move(host), port);
      }

      ~kernel() { socket.close(); }

      boost::asio::io_context io;
      boost::asio::ip::udp::socket socket;
      boost::asio::ip::udp::endpoint endpoint;
    };

   public:
    using char_type = char;
    using category  = boost::iostreams::sink_tag;

    explicit sink(std::string host, std::uint16_t port) : _out_kernel{std::make_shared<kernel>(std::move(host), port)} {}

    sink()                       = delete;
    sink(const sink&)            = default;
    sink& operator=(const sink&) = delete;
    sink(sink&&)                 = default;
    sink& operator=(sink&&)      = default;

    ~sink() {}

    [[nodiscard]] std::streamsize write(const char* s, std::streamsize n) {
      return _out_kernel->socket.send_to(boost::asio::buffer(s, n), _out_kernel->endpoint);
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

    std::shared_ptr<kernel> _out_kernel;
  };
}  // namespace detail

using ostreambuf = boost::iostreams::stream_buffer<detail::sink>;
class ostream : public boost::iostreams::stream<detail::sink> {
 public:
  explicit ostream(std::string host, std::uint16_t port) : boost::iostreams::stream<detail::sink>{std::move(host), port} {}
};

struct flush_t {};
constexpr auto flush = flush_t{};

inline std::ostream& operator<<(std::ostream& os, const flush_t&) {
  os.flush();
  return os;
}

template <contiguous_byte_range_like Range_T>
  requires(not std::is_same_v<std::string, std::decay_t<Range_T>>)
ostream& operator<<(ostream& os, Range_T&& bytes) {
  if (not bytes.empty()) {
    os.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  }

  return os;
}

}  // namespace nsl::udp
