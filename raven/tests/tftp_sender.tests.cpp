#include "tftp_sender.hpp"

#include <catch2/catch_test_macros.hpp>
#include <wite/io.hpp>

namespace {
std::uint16_t get_op_code(const raven::tftp::packet& pkt) {
  return wite::io::read<wite::io::big_endian<std::uint16_t>>(pkt);
}
}  // namespace

TEST_CASE("TFTP Sender tests") {
  SECTION("write request has the correct op code") {
    REQUIRE(std::uint16_t{2} == get_op_code(raven::tftp::make_write_request("foo")));
  }
}
