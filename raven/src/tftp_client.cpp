#include "tftp_client.hpp"

#include <wite/io.hpp>

namespace raven::tftp {

packet make_write_request(std::string_view filename) {
  auto out = packet{};

  wite::io::write(out, wite::io::big_endian{std::uint16_t{2}}, filename);

  return out;
}
}  // namespace raven::tftp
