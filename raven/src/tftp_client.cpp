#include "tftp_client.hpp"

#include <wite/io.hpp>

#include <string_view>

using namespace std::string_view_literals;

namespace raven::tftp {

packet_bytes make_write_request_bytes(std::string_view filename) {
  auto pkt = packet_bytes{};

  wite::io::write(pkt, wite::io::big_endian{std::uint16_t{2}}, filename, '\0', "octet"sv, '\0');

  return pkt;
}
}  // namespace raven::tftp
