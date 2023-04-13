#include "framework.h"

#include <nsl/udp/istream.hpp>
#include <nsl/udp/ostream.hpp>
#include <nsl/udp/types.hpp>

#include "test/io_runner.hpp"
#include "test/waiting.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <nlohmann/json.hpp>
#include <boost/asio.hpp>
#include <wite/collections/make_vector.hpp>

#include <algorithm>
#include <random>
#include <thread>
#include <vector>

using nlohmann::json;
using namespace nsl;

TEST_CASE("reading and writing to UDP streams") {
  auto io = boost::asio::io_context{};

  constexpr auto test_port = udp::port_number{40000};

  SECTION("blocking streams") {
    auto udp_in  = udp::istream{io, test_port};
    auto udp_out = udp::ostream{"localhost", test_port};

    SECTION("writing and reading JSON") {
      auto sent_json = json::parse(R"({"bool_field": true, "int_field": 12345, "string_field": "ahoy there!"})");

      auto recv_data = test::running_async([&]() {
        auto recv_json = json{};
        udp_in >> recv_json;
        return recv_json;
      });

      udp_out << sent_json << std::endl;

      REQUIRE(sent_json == recv_data.get());
    }

    SECTION("writing and reading 1024 bytes") {
      auto sent_bytes = wite::make_vector<std::byte>(wite::arg::reserve{1024});

      auto rng         = std::mt19937{345234};  // arbitrary seed.
      std::generate_n(std::back_inserter(sent_bytes), 1024, [&]() {
        return static_cast<std::byte>(std::uniform_int<>{0x00, 0xFF}(rng));
      });

      auto recv_bytes = test::running_async([&]() {
        auto recv_bytes = std::vector<std::byte>(sent_bytes.size(), std::byte{0});
        udp_in >> recv_bytes;
        return recv_bytes;
      });

      udp_out << sent_bytes << udp::flush;

      REQUIRE(std::ranges::equal(sent_bytes, recv_bytes.get()));
    }
  }
}