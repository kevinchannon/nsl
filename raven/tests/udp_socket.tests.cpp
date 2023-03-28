#include "framework.h"

#include "udp_input_socket.hpp"
#include "udp_output_socket.hpp"

#include "io_runner.hpp"

#include <catch2/catch_test_macros.hpp>

#include <boost/asio.hpp>

#include <vector>
#include <future>
#include <random>
#include <sstream>

TEST_CASE("UDP socket tests") {
  auto io              = boost::asio::io_context{};
  const auto test_port = std::uint16_t{40000};

  SECTION("udp_output_socket::create creates an input socket") {
    REQUIRE(nullptr != raven::udp_output_socket::create(io, "localhost", test_port));
  }

  auto buffer        = std::vector<char>{};
  auto received_data = std::string{};
  auto process_data  = [&received_data, &buffer](auto byte_count) {
    std::copy_n(buffer.begin(), byte_count, std::back_inserter(received_data));
  };

  GIVEN("some small data") {
    auto data = std::stringstream{"ahoy-hoy!"};

    WHEN("I send and receive the data") {
      buffer.resize(128);
      auto input_socket = raven::udp_input_socket::create(io, test_port, buffer, std::move(process_data));
      auto io_runner    = test::io_runner{io};

      std::ignore = raven::udp_output_socket::create(io, "localhost", test_port)->send(data);

      THEN("The received data is the same as the data that I sent") REQUIRE(data.str() == received_data);
    }
  }

  GIVEN("Some data larger than the send buffer size") {
    auto data = std::stringstream{};
    auto rng  = std::mt19937_64{934853};  // arbitrary seed.
    std::generate_n(std::ostreambuf_iterator<char>{data}, 1025, [&rng]() {
      return static_cast<char>(std::uniform_int_distribution<std::int32_t>{30, 39}(rng));
    });

    WHEN("I send and receive the data") {
      buffer.resize(1024);
      auto input_socket = raven::udp_input_socket::create(io, test_port, buffer, std::move(process_data));
      auto io_runner    = test::io_runner{io};

      std::ignore = raven::udp_output_socket::create(io, "localhost", test_port)->send(data);

      THEN("The received data is the same as the data that I sent")
        REQUIRE(data.str() == received_data);
    }
  }
}
