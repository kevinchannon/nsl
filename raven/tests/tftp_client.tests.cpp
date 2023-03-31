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

      auto expected_write_req = std::stringstream{};
      expected_write_req << tftp::write_request{"foo"};

      auto expected_data = std::string{};
      std::copy(std::istreambuf_iterator{expected_write_req}, {}, std::back_inserter(expected_data));

      REQUIRE(expected_data == sent_data);
    }
  }
}
