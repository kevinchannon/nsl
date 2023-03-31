#include "tftp_client.hpp"
#include "tftp_types.hpp"

#include <wite/io.hpp>

#include <string_view>
#include <tuple>

using namespace std::string_view_literals;

namespace raven::tftp {

void client::send(std::string_view filename, std::istream& ) {
  auto write_req = std::stringstream{};
  write_req << write_request{filename};

  std::ignore = _udp_emitter->send(write_req);
}

std::stringstream client::_insert_write_request(std::stringstream&& stream, std::string_view) {
  return std::move(stream);
}

}  // namespace raven::tftp
