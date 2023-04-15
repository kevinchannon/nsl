#pragma once

#include "types.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream.hpp>

#include <wite/core/scope.hpp>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <istream>
#include <memory>
#include <mutex>
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
        if (socket.is_open()) {
          socket.close();
        }
      }

      boost::asio::io_context& io;
      boost::asio::ip::udp::socket socket;
      static constexpr auto recv_buf_size = 4096;
      boost::asio::streambuf recv_data{};
      std::atomic_bool async_read_in_progress{false};
      std::atomic_bool sync_read_in_progress{false};
      std::mutex mtx;
      std::condition_variable exiting_async_read;
    };

   public:
    using char_type = char;
    using category  = boost::iostreams::source_tag;

    explicit source(boost::asio::io_context& io, port_number port)
        : _in_kernel{std::make_shared<kernel>(io, _resolve_endpoint(io, "0.0.0.0", port))} {}

    source()                         = delete;
    source(const source&)            = default;
    source& operator=(const source&) = delete;
    source(source&&)                 = default;
    source& operator=(source&&)      = default;

    ~source() {}

    [[nodiscard]] std::streamsize read(char* s, std::streamsize n) {
      if (_in_kernel->async_read_in_progress) {
        return -1;
      }

      _in_kernel->sync_read_in_progress = true;
      auto _                         = wite::scope_exit{[this]() { _in_kernel->sync_read_in_progress = false; }};

      auto bytes_recvd = _in_kernel->socket.receive(boost::asio::buffer(s, n));

      return bytes_recvd;
    }

    template <async_recv_fn_like Callback_T>
    bool async_read([[maybe_unused]] Callback_T&& callback) {
      if (_in_kernel->sync_read_in_progress.load()) {
        return false;
      }

      _in_kernel->async_read_in_progress = true;
      auto recv_buf                   = _in_kernel->recv_data.prepare(_in_kernel->recv_buf_size);

      _in_kernel->socket.async_receive(recv_buf, [this, cb = std::forward<Callback_T>(callback)](auto&& err, auto&& n) {
        if (err) {
          { auto lock = std::unique_lock{_in_kernel->mtx}; }
          _in_kernel->exiting_async_read.notify_all();
          return;
        }

        async_read(_do_receive_and_handle_data(std::move(cb), n));
      });

      return true;
    }

    void cancel_async_read() {
      if (not _in_kernel->async_read_in_progress.load()) {
        return;
      }

      _do_cancel_async_read();
    }

    void cancel_sync_read() {
      if (not _in_kernel->sync_read_in_progress.load()) {
        return;
      }

      _do_cancel_sync_read();
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

    template <async_recv_fn_like Callback_T>
    [[nodiscard]] Callback_T _do_receive_and_handle_data(Callback_T callback, size_t n) {
      _in_kernel->recv_data.commit(n);

      auto data_stream = std::istream{&_in_kernel->recv_data};
      callback(data_stream, n);

      _in_kernel->recv_data.consume(n);

      return callback;
    }

    void _do_cancel_async_read() {
      auto lock = std::unique_lock{_in_kernel->mtx};
      _in_kernel->socket.cancel();
      _in_kernel->exiting_async_read.wait(lock);

      // The last point that we have visibility on the process is marked by the condition variable, however
      // there is still stuff to happen after that point, so we wait for a bit.
      // Find a wait to NOT DO THIS.
      std::this_thread::sleep_for(std::chrono::milliseconds{2});

      _in_kernel->async_read_in_progress = false;
    }

    void _do_cancel_sync_read() {
      auto lock = std::unique_lock{_in_kernel->mtx};
      _in_kernel->socket.cancel();

      std::this_thread::sleep_for(std::chrono::milliseconds{2});

      _in_kernel->sync_read_in_progress = false;
    }

    std::shared_ptr<kernel> _in_kernel;
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

  void cancel_sync_recv() {
    (*this)->cancel_sync_read();

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
  if (not is->async_read(std::forward<Callback_T>(callback))) {
    is.setstate(std::ios::failbit);
  }

  return is;
}

}  // namespace nsl::udp
