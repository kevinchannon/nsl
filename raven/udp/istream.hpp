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
#include <span>

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

    template<async_recv_fn_like Callback_T>
    void async_read([[maybe_unused]] Callback_T&& callback) {
      auto recv_buf = _recv_data.prepare(_recv_buf_size);
      _socket.async_receive_from(recv_buf, _endpoint, [this](auto&& err, auto&& n) {
        if (err) {
          return;
        }

        _recv_data.commit(n);

        auto data_stream = std::istream{&_recv_data};
        callback(data_stream, n);

        _recv_data.consume(n);
        // async_read(callback);
      });
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
class istream : public boost::iostreams::stream<detail::source> {
 public:
  istream(boost::asio::io_context& io, port_number port) : boost::iostreams::stream<detail::source>{io, port} {}
};

template <contiguous_byte_range_like Range_T>
  requires(not std::is_same_v<std::string, std::decay_t<Range_T>>)
istream& operator>>(istream& is, Range_T&& bytes) {
  if (not bytes.empty()) {
    is.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
  }

  return is;
}

template <async_recv_fn_like Callback_T>
istream& operator>>(istream& is, Callback_T&& callback) {
  is->async_read(std::forward<Callback_T>(callback));
}

}  // namespace raven::udp
