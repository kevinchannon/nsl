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
      kernel(boost::asio::io_context& io, std::string host, port_number port)
          : io{io}, socket{io, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 0)}, endpoint{} {
        endpoint = sink::_resolve_endpoint(io, std::move(host), port);
      }

      ~kernel() {
        if (socket.is_open()) {
          socket.close();
        }
      }

      boost::asio::io_context& io;
      boost::asio::ip::udp::socket socket;
      boost::asio::ip::udp::endpoint endpoint;
    };

   public:
    using char_type = char;
    using category  = boost::iostreams::sink_tag;

    explicit sink(boost::asio::io_context& io, std::string host, std::uint16_t port)
        : _out_kernel{std::make_shared<kernel>(io, std::move(host), port)} {}

    sink()                       = delete;
    sink(const sink&)            = default;
    sink& operator=(const sink&) = delete;
    sink(sink&&)                 = default;
    sink& operator=(sink&&)      = default;

    ~sink() {}

    [[nodiscard]] std::streamsize write(const char* s, std::streamsize n) {
      return _out_kernel->socket.send_to(boost::asio::buffer(s, n), _out_kernel->endpoint);
    }

    template <typename Data_T, async_send_fn_like Callback_T>
    void async_write([[maybe_unused]] std::pair<Data_T, Callback_T>&& data_and_callback) {

      // _out_kernel->socket.set_option(boost::asio::socket_base::send_buffer_size{static_cast<int>(data_and_callback.first.size())});

      auto data_to_send = std::make_shared<Data_T>(std::move(data_and_callback.first));
      auto send_buf     = boost::asio::buffer(*data_to_send);
      _out_kernel->socket.async_send_to(
          send_buf,
          _out_kernel->endpoint,
          [data = std::move(data_to_send), callback = std::move(data_and_callback.second)](auto&& ec, size_t n) {
            if (not ec) {
              callback(n);
            }
          });
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
  explicit ostream(boost::asio::io_context& io, std::string host, std::uint16_t port)
      : boost::iostreams::stream<detail::sink>{io, std::move(host), port} {}
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

template <typename Data_T, async_send_fn_like Callback_T>
ostream& operator<<(ostream& os, std::pair<Data_T, Callback_T>&& data_and_callback) {
  os->async_write(std::forward<std::pair<Data_T, Callback_T>>(data_and_callback));
  return os;
}

}  // namespace nsl::udp
