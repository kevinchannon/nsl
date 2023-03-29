#include "tftp_sender.hpp"

#include <wite/io.hpp>

namespace raven::tftp {

packet make_write_request(std::string_view ) {
  auto out = packet{};

  wite::io::write(out, wite::io::big_endian{std::uint16_t{2}});

  return out;
}
}  // namespace raven::tftp
