#include "tftp_client.hpp"

#include <catch2/catch_test_macros.hpp>
#include <wite/io.hpp>

#include <string_view>

using namespace std::string_view_literals;

namespace {
std::uint16_t get_op_code(const raven::tftp::packet_bytes& pkt) {
  return wite::io::read<wite::io::big_endian<std::uint16_t>>(pkt);
}

std::string_view get_write_request_filename(const raven::tftp::packet_bytes& pkt) {
  return {pkt.data() + 2, std::find(pkt.data() + 2, pkt.data() + pkt.size() - 2, '\0')};
}

std::string_view get_write_request_mode(const raven::tftp::packet_bytes& pkt, std::ptrdiff_t offset) {
  return {pkt.data() + offset, std::find(pkt.data() + offset, pkt.data() + pkt.size() - offset, '\0')};
}

}  // namespace

TEST_CASE("TFTP Sender tests") {
  SECTION("write request has the correct op code") {
    REQUIRE(std::uint16_t{2} == get_op_code(raven::tftp::make_write_request_bytes("foo")));
  }

  SECTION("write request has the correct filename") {
    REQUIRE("foo" == std::string{get_write_request_filename(raven::tftp::make_write_request_bytes("foo"))});
  }

  SECTION("write request has 'octet' mode") {
    constexpr auto mode_offset = 2 + 3 + 1; // op code size + "foo" + null character
    REQUIRE("octet" == std::string{get_write_request_mode(raven::tftp::make_write_request_bytes("foo"), mode_offset)});
  }
}
