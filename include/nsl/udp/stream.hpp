#pragma once

#include <nsl/udp/istream.hpp>
#include <nsl/udp/ostream.hpp>

namespace nsl::udp {

class stream : public istream, public ostream {
 public:
  explicit stream(boost::asio::io_context& io, port_number local_port, std::string remote_host, port_number remote_port)
      : istream{io, local_port}, ostream{std::move(remote_host), remote_port} {}
};

}  // namespace nsl::udp
