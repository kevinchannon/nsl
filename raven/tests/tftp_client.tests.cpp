#include "tftp_client.hpp"

#include <catch2/catch_test_macros.hpp>
#include <wite/io.hpp>

#include <string_view>

using namespace std::string_view_literals;

namespace {
std::uint16_t get_op_code(const raven::tftp::packet& pkt) {
  return wite::io::read<wite::io::big_endian<std::uint16_t>>(pkt);
}

std::string_view get_write_request_filename(const raven::tftp::packet& pkt) {
  return {pkt.data() + 2, std::find(pkt.data() + 2, pkt.data() + pkt.size() - 2, '\0')};
}

}  // namespace

TEST_CASE("TFTP Sender tests") {
  SECTION("write request has the correct op code") {
    REQUIRE(std::uint16_t{2} == get_op_code(raven::tftp::make_write_request("foo")));
  }

  SECTION("write request has the correct filename") {
    REQUIRE("foo" == std::string{get_write_request_filename(raven::tftp::make_write_request("foo"))});
  }
}
