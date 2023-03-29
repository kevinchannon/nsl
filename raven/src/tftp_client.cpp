#include "tftp_client.hpp"

#include <wite/io.hpp>

#include <string_view>

using namespace std::string_view_literals;

namespace raven::tftp {

packet make_write_request(std::string_view filename) {
  auto out = packet{};

  wite::io::write(out, wite::io::big_endian{std::uint16_t{2}}, filename, '\0', "octet"sv, '\0');

  return out;
}
}  // namespace raven::tftp
