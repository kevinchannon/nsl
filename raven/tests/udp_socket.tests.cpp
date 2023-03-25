#include "udp_output_socket.hpp"
#include "udp_input_socket.hpp"

#include <catch2/catch_all.hpp>

#include <sstream>

TEST_CASE("UDP socket tests") {
  SECTION("udp_output_socket::create creates an input socket") {
    REQUIRE(nullptr != raven::udp_output_socket::create("0.0.0.0", 40000));
  }

  GIVEN("some small data") {
    auto data = std::stringstream{"ahoy-hoy!"};

    WHEN("I send the data") {
      const auto result = raven::udp_output_socket::create("0.0.0.0", 40000)->send(data);
      THEN("the result is a success") REQUIRE(0 == result);
    }

    WHEN("I send and receive the data") {
      auto input_socket = raven::udp_input_socket::create(40000);
      std::ignore       = raven::udp_output_socket::create("0.0.0.0", 40000)->send(data);

      auto received = std::stringstream{};
      REQUIRE(0 == input_socket->recv(received));

      THEN("The received data is the same as the data that I sent") REQUIRE(data.str() == received.str());
    }
  }
}
