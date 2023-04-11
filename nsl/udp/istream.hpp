#pragma once

#include "framework.h"

#include "types.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream.hpp>

#include <atomic>
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
  class source {
    struct kernel {
      kernel(boost::asio::io_context& io, boost::asio::ip::udp::endpoint endpoint) : io{io}, socket{io, endpoint} {}

      ~kernel() {
        socket.shutdown(boost::asio::ip::udp::socket::shutdown_both);
        socket.close();
      }

      boost::asio::io_context& io;
      boost::asio::ip::udp::socket socket;
      static constexpr auto recv_buf_size = 4096;
      boost::asio::streambuf recv_data{};
      std::atomic_bool async_read_in_progress{false};
      std::mutex mtx;
      std::condition_variable exiting_async_read;
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
      if (_kernel->async_read_in_progress) {
        return -1;
      }

      auto bytes_recvd = _kernel->socket.receive(boost::asio::buffer(s, n));

      return bytes_recvd;
    }

    template <async_recv_fn_like Callback_T>
    void async_read([[maybe_unused]] Callback_T&& callback) {
      _kernel->async_read_in_progress = true;
      auto recv_buf                   = _kernel->recv_data.prepare(_kernel->recv_buf_size);
      _kernel->socket.async_receive(recv_buf, [this, &callback](auto&& err, auto&& n) {
        if (err) {
          auto lock = std::unique_lock{_kernel->mtx};
          _kernel->exiting_async_read.notify_all();
          return;
        }

        _kernel->recv_data.commit(n);

        auto data_stream = std::istream{&_kernel->recv_data};
        callback(data_stream, n);

        _kernel->recv_data.consume(n);
        async_read(callback);
      });
    }

    void cancel_async_read() {
      if (not _kernel->async_read_in_progress) {
        return;
      }

      _do_cancel_async_read();
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

    void _do_cancel_async_read() {
      auto lock = std::unique_lock{_kernel->mtx};
      _kernel->socket.cancel();
      _kernel->exiting_async_read.wait(lock);

      // The last point that we have visibility on the process is marked by the condition variable, however
      // there is still stuff to happen after that point, so we wait for a bit.
      // Find a wait to NOT DO THIS.
      std::this_thread::sleep_for(std::chrono::milliseconds{2});

      _kernel->async_read_in_progress = false;
    }

    std::shared_ptr<kernel> _kernel;
  };
}  // namespace detail

using istreambuf = boost::iostreams::stream_buffer<detail::source>;
class istream : public boost::iostreams::stream<detail::source> {
 public:
  explicit istream(boost::asio::io_context& io, port_number port) : boost::iostreams::stream<detail::source>{io, port} {}

  void cancel_async_recv() {
    (*this)->cancel_async_read();
    
    // Clear any error that previously occurred on the stream, since someone may have tried to synchronously read the stream
    // while the async read was in progress. This will have put the stream in an error state and it will not be possible to
    // read from the stream until that error is cleared.
    clear();
  }
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
  return is;
}

}  // namespace nsl::udp
