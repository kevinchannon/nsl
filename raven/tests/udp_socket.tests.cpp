#include "udp_output_socket.hpp"
#include "udp_input_socket.hpp"

#include <catch2/catch_all.hpp>

#include "framework.h"
#include <boost/asio.hpp>

#include <sstream>
#include <array>
#include <future>

TEST_CASE("UDP socket tests") {

  auto io = boost::asio::io_context{};

  SECTION("udp_output_socket::create creates an input socket") {
    REQUIRE(nullptr != raven::udp_output_socket::create(io, "localhost", 1337));
  }

  GIVEN("some small data") {
    auto data          = std::stringstream{"ahoy-hoy!"};
    auto buffer        = std::array<char, 128>{};
    auto received_data = std::string{};
    auto process_data  = [&received_data, &buffer](auto byte_count) {
      std::copy_n(buffer.begin(), byte_count, std::back_inserter(received_data));
    };

    WHEN("I send and receive the data") {
      auto input_socket = raven::udp_input_socket::create(io, 1337, buffer, std::move(process_data));
      auto _ = std::async([&io]() { io.run(); });

      std::ignore = raven::udp_output_socket::create(io, "localhost", 1337)->send(data);

      THEN("The received data is the same as the data that I sent") REQUIRE(data.str() == received_data);

      input_socket->stop();
    }
  }

  io.stop();
}
