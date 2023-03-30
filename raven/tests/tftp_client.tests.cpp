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

TEST_CASE("make_write_request_bytes tests") {
  using namespace raven;

  SECTION("write request has the correct op code") {
    REQUIRE(std::uint16_t{2} == get_op_code(tftp::make_write_request_bytes("foo").first));
  }

  SECTION("write request has the correct filename") {
    REQUIRE("foo" == std::string{get_write_request_filename(tftp::make_write_request_bytes("foo").first)});
  }

  SECTION("write request has 'octet' mode") {
    constexpr auto mode_offset = 2 + 3 + 1;  // op code size + "foo" + null character
    REQUIRE("octet" == std::string{get_write_request_mode(tftp::make_write_request_bytes("foo").first, mode_offset)});
  }

  SECTION("write request has the correct size") {
    REQUIRE(13 == tftp::make_write_request_bytes("foo").second);
  }
}

TEST_CASE("TFTP Client tests") {
  using namespace raven;

  auto udp_emitter = std::make_unique<test::udp::emitter_mock>();

  SECTION("send method") {
    auto sent_data       = std::string{};
    udp_emitter->send_fn = [&](std::istream& data) {
      std::getline(data, sent_data);
      return sent_data.size();
    };

    SECTION("first end sends a write request") {
      auto data = std::stringstream{"some data"};
      tftp::client{std::move(udp_emitter)}.send("foo", data);

      REQUIRE(sent_data == "some data");
    }
  }
}
