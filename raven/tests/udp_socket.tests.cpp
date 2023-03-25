#include "udp_output_socket.hpp"
#include "udp_input_socket.hpp"

#include <catch2/catch_all.hpp>

#include <sstream>

TEST_CASE("UDP socket tests") {
  SECTION("udp_output_socket::create creates an input socket") {
    REQUIRE(nullptr != raven::udp_output_socket::create("0.0.0.0", 69));
  }

  SECTION("send...") {
    SECTION("returns 0 when sending some valid data") {
      auto data = std::stringstream{"ahoy-hoy!"};
      REQUIRE(0 == raven::udp_output_socket::create("0.0.0.0", 69)->send(data));
    }
  }
}
