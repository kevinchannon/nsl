#include "tftp_client.hpp"

#include <wite/io.hpp>

#include <string_view>
#include <tuple>

using namespace std::string_view_literals;

namespace raven::tftp {

void client::send(std::string_view filename, std::istream& ) {
  auto write_req = _insert_write_request(std::stringstream{}, filename);
  std::ignore = _udp_emitter->send(write_req);
}

std::stringstream client::_insert_write_request(std::stringstream&& stream, std::string_view filename) {

  wite::io::write(stream, );
  return std::move(stream);
}

}  // namespace raven::tftp
