#include "tftp_client.hpp"
#include "udp_mocks.hpp"

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

TEST_CASE("TFTP Client tests") {
  using namespace raven;

  SECTION("connect method") {
    SECTION("sends a write request") {
      auto expected_write_req = std::stringstream{};
      expected_write_req << tftp::write_request{"foo"};

      auto expected_data = std::vector<char>{};
      std::copy(std::istreambuf_iterator{expected_write_req}, {}, std::back_inserter(expected_data));

      auto target = std::stringstream{};
      auto client = tftp::client{};
      client.connect(target, "foo");

      auto received_data = std::vector<char>{};
      std::copy(std::istreambuf_iterator{target}, {}, std::back_inserter(received_data));

      REQUIRE(expected_data == received_data);
    }
  }
}
