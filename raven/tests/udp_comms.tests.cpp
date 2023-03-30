#include "framework.h"

#include "udp_emitter.hpp"
#include "udp_receiver.hpp"

#include "io_runner.hpp"
#include "waiting.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <boost/asio.hpp>

#include <chrono>
#include <future>
#include <random>
#include <sstream>
#include <thread>
#include <vector>
#include <string_view>
#include <iostream>

using namespace std::chrono_literals;

TEST_CASE("UDP socket tests") {
  using namespace raven;

  auto io              = boost::asio::io_context{};
  const auto test_port = std::uint16_t{40000};

  SECTION("udp_output_socket::create creates an input socket") {
    REQUIRE(nullptr != raven::udp::emitter::create("localhost", test_port));
  }

  auto buffer = std::vector<char>{};

  auto received_data = std::string{};
  auto mtx           = std::mutex{};
  auto process_data  = [&](auto byte_count) {
    auto lock = std::lock_guard<std::mutex>{mtx};
    std::copy_n(buffer.begin(), byte_count, std::back_inserter(received_data));
  };

  GIVEN("some small data") {
    auto data = std::stringstream{"ahoy-hoy!"};

    WHEN("I send and receive the data") {
      buffer.resize(128);
      auto input_socket = raven::udp::receiver::create(io, test_port, buffer, std::move(process_data));
      auto io_runner    = test::io_runner{io};

      REQUIRE(std::string_view{"ahoy-hoy!"}.length() == raven::udp::emitter::create("localhost", test_port)->send(data));

      THEN("The received data is the same as the data that I sent") {
        const auto all_data_received = [&]() {
          auto lock = std::lock_guard<std::mutex>{mtx};
          return received_data.size() == std::string_view{"ahoy-hoy!"}.length();
        };
        REQUIRE(test::wait_for(all_data_received, 50ms));
        REQUIRE(data.str() == received_data);
      }
    }
  }

  GIVEN("Some data around the send buffer size") {
    auto data = std::stringstream{};
    auto rng  = std::mt19937_64{934853};  // arbitrary seed.

    const auto data_size = GENERATE(1022, 1023, 1024, 1025, 1026);

    std::generate_n(std::ostreambuf_iterator<char>{data}, data_size, [&rng]() {
      return static_cast<char>(std::uniform_int_distribution<std::int32_t>{0x30, 0x39}(rng));
    });

    WHEN(std::format("I send {} bytes of data", data_size)) {
      buffer.resize(1024);
      auto input_socket = raven::udp::receiver::create(io, test_port, buffer, std::move(process_data));
      auto io_runner    = test::io_runner{io};

      REQUIRE(data_size == raven::udp::emitter::create("localhost", test_port)->send(data));

      THEN("The received data is the same as the data that I sent") {
        const auto all_data_received = [&]() {
          auto lock = std::lock_guard<std::mutex>{mtx};
          return received_data.size() == data_size;
        };
        REQUIRE(test::wait_for(all_data_received, 50ms));
        REQUIRE(data.str() == received_data);
      }
    }
  }

  GIVEN("Some large data") {
    auto data = std::stringstream{};
    auto rng  = std::mt19937_64{1110394};  // arbitrary seed.
    constexpr auto data_size = size_t{64 * 1024};
    std::generate_n(std::ostreambuf_iterator<char>{data}, data_size, [&rng]() {
      return static_cast<char>(std::uniform_int_distribution<std::int32_t>{0x30, 0x39}(rng));
    });

    WHEN(std::format("I send the data", data_size)) {
      buffer.resize(1024);
      auto input_socket = raven::udp::receiver::create(io, test_port, buffer, std::move(process_data));
      auto io_runner    = test::io_runner{io};

      REQUIRE(data_size == raven::udp::emitter::create("localhost", test_port)->send(data));

      THEN("The received data is the same as the data that I sent") {
        const auto all_data_received = [&]() {
          auto lock = std::lock_guard<std::mutex>{mtx};
          return received_data.size() == data_size;
        };
        REQUIRE(test::wait_for(all_data_received, 1s));
        REQUIRE(data.str() == received_data);
      }
    }
  }
}
