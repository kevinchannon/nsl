#pragma once

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/stream.hpp>

#include <cstdint>
#include <istream>
#include <memory>
#include <span>
#include <string>

namespace boost::asio {
class io_context;
}

namespace raven::udp {

struct emitter {
  static std::unique_ptr<emitter> create(std::string ip, std::uint16_t port);

  virtual ~emitter() = default;

  [[nodiscard]] virtual size_t send(std::istream& data)                        = 0;
  [[nodiscard]] virtual std::streamsize send(const char* s, std::streamsize n) = 0;
};

class sink {
 public:
  using char_type = char;
  using category  = boost::iostreams::sink_tag;

  explicit sink(std::string host, std::uint16_t port)
      : _io{}, _socket{_io, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 0)}, _endpoint{} {
    _endpoint = _resolve_endpoint(_io, std::move(host), port);
  }

  ~sink() { _socket.close(); }

  [[nodiscard]] std::streamsize write(const char* s, std::streamsize n) {
    return _socket.send_to(boost::asio::buffer(s, n), _endpoint);
  }

 private:
  [[nodiscard]] static boost::asio::ip::udp::endpoint _resolve_endpoint(boost::asio::io_context& io,
                                                                        std::string host,
                                                                        std::uint16_t port) {
    boost::asio::ip::udp::resolver resolver(io);
    boost::asio::ip::udp::resolver::query query(
        boost::asio::ip::udp::v4(), host, std::to_string(static_cast<std::uint32_t>(port)));
    return *resolver.resolve(query);
  }

  boost::asio::io_context _io;
  boost::asio::ip::udp::socket _socket;
  boost::asio::ip::udp::endpoint _endpoint;
};

class ostreambuf : public boost::iostreams::stream_buffer<sink> {
 public:
  explicit ostreambuf(std::string host, std::uint16_t port)
      : boost::iostreams::stream_buffer<sink>{sink{std::move(host), port}} {}
};

class ostream : public boost::iostreams::stream<sink> {
 public:
  explicit ostream(std::string host, std::uint16_t port) : boost::iostreams::stream<sink>{sink{std::move(host), port}} {}
};

}  // namespace raven::udp
