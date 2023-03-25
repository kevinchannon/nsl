#include "udp_output_socket.hpp"
#include "udp_input_socket.hpp"

#include <catch2/catch_all.hpp>

#include <sstream>

TEST_CASE("UDP socket tests") {
  SECTION("udp_output_socket::create creates an input socket") {
    REQUIRE(nullptr != raven::udp_output_socket::create("0.0.0.0", 69));
  }

  GIVEN("some small data") {
    auto data = std::stringstream{"ahoy-hoy!"};

    WHEN("I send the data") {
      const auto result = raven::udp_output_socket::create("0.0.0.0", 69)->send(data);
      
      THEN("the result is a success") REQUIRE(0 == result);
    }
  }
}
