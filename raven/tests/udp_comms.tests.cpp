#include "framework.h"

#include "udp_emitter.hpp"
#include "udp_receiver.hpp"

#include "io_runner.hpp"
#include "waiting.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <boost/asio.hpp>

#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <random>
#include <sstream>
#include <string_view>
#include <thread>
#include <vector>

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
  auto received_size = std::atomic<size_t>{0};
  auto process_data  = [&](auto& recv_buf, auto byte_count) {
    std::copy_n(std::istreambuf_iterator{recv_buf}, byte_count, std::back_inserter(received_data));
    received_size += byte_count;
  };

  GIVEN("some small data") {
    auto data = std::stringstream{"ahoy-hoy!"};

    WHEN("I send and receive the data") {
      buffer.resize(128);
      auto input_socket = raven::udp::receiver::create(io, test_port, std::move(process_data));
      auto io_runner    = test::io_runner{io};

      REQUIRE(std::string_view{"ahoy-hoy!"}.length() == raven::udp::emitter::create("localhost", test_port)->send(data));

      THEN("The received data is the same as the data that I sent") {
        REQUIRE(test::wait_for([&]() { return received_size.load() == std::string_view{"ahoy-hoy!"}.length(); }, 50ms));
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
      auto input_socket = raven::udp::receiver::create(io, test_port, std::move(process_data));
      auto io_runner    = test::io_runner{io};

      REQUIRE(data_size == raven::udp::emitter::create("localhost", test_port)->send(data));

      THEN("The received data is the same as the data that I sent") {
        REQUIRE(test::wait_for([&]() { return received_size.load() == data_size; }, 50ms));
        REQUIRE(data.str() == received_data);
      }
    }
  }

  GIVEN("Some large data") {
    auto data                = std::stringstream{};
    auto rng                 = std::mt19937_64{1110394};  // arbitrary seed.
    constexpr auto data_size = size_t{64 * 1024};
    std::generate_n(std::ostreambuf_iterator<char>{data}, data_size, [&rng]() {
      return static_cast<char>(std::uniform_int_distribution<std::int32_t>{0x30, 0x39}(rng));
    });

    WHEN(std::format("I send the data", data_size)) {
      buffer.resize(1024);
      auto input_socket = raven::udp::receiver::create(io, test_port, std::move(process_data));
      auto io_runner    = test::io_runner{io};

      REQUIRE(data_size == raven::udp::emitter::create("localhost", test_port)->send(data));

      THEN("The received data is the same as the data that I sent") {
        REQUIRE(test::wait_for([&]() { return received_size.load() == data_size; }, 50ms));
        REQUIRE(data.str() == received_data);
      }
    }
  }
}

TEST_CASE("UDP ostream") {
  using namespace raven;

  auto rng             = std::mt19937_64{1110394};  // arbitrary seed.
  auto io              = boost::asio::io_context{};
  const auto test_port = std::uint16_t{40000};

  const auto data_size = GENERATE(size_t{0x10},
                                  size_t{0x20},
                                  size_t{0x80},
                                  size_t{0x100},
                                  size_t{0x200},
                                  size_t{0x800},
                                  size_t{0x1000},
                                  size_t{0x2000},
                                  size_t{0x8000},
                                  size_t{0x10000},
                                  size_t{0x20000},
                                  size_t{0x80000},
                                  size_t{0x100000},
                                  size_t{0x200000},
                                  size_t{0x800000} );

  SECTION(std::format("sends and receives {} bytes of data", data_size)) {
    auto data = std::string(data_size, '\0');
    std::generate_n(data.begin(), data.size(), [&rng]() {
      return static_cast<char>(std::uniform_int_distribution<std::int32_t>{0x30, 0x39}(rng));
    });

    auto buffer = std::vector<char>{};

    auto received_data = std::string(data_size, '\0');
    auto write_pos     = received_data.begin();
    auto received_size = std::atomic<size_t>{0};
    auto process_data  = [&](auto& recv_buf, auto byte_count) {
      std::copy_n(std::istreambuf_iterator<char>{recv_buf}, byte_count, write_pos);
      std::advance(write_pos, byte_count);
      received_size += byte_count;
    };

    buffer.resize(4096);
    auto input_socket = raven::udp::receiver::create(io, test_port, std::move(process_data), 25 * 1024 * 1024);
    auto io_runner    = test::io_runner{io};

    auto udp_stream = udp::ostream{"localhost", test_port};
    udp_stream << data << udp::flush;

    if (not test::wait_for([&]() { return received_size.load() == data_size; }, 3s)) {
      std::cerr << "Received " << received_size.load() << " bytes. Expected " << data_size << std::endl;
      REQUIRE(false);
    }

    REQUIRE(data == received_data);
  }
}
