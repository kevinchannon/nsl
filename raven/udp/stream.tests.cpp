#include "framework.h"

#include "udp/istream.hpp"
#include "udp/ostream.hpp"
#include "udp/types.hpp"

#include "test/io_runner.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <nlohmann/json.hpp>

#include <boost/asio.hpp>

using nlohmann::json;
using namespace raven;

TEST_CASE("reading and writing to UDP streams") {
  auto io        = boost::asio::io_context{};
  constexpr auto test_port = udp::port_number{6078};

  SECTION("blocking streams") {
    SECTION("Writing and reading JSON") {
      auto udp_in        = udp::istream{io, test_port};
      auto udp_out       = udp::ostream{"localhost", test_port};

      auto recv_json = json{};
      auto data_received = std::async([&]() { udp_in >> recv_json; });

      auto sent_json = json::parse(R"({"bool_field": true, "int_field": 12345, "string_field": "ahoy there!"})");

      udp_out << sent_json << std::endl;

      data_received.wait();

      REQUIRE(sent_json == recv_json);
    }
  }
}