#include "framework.h"

#include <nsl/udp/istream.hpp>
#include <nsl/udp/ostream.hpp>
#include <nsl/udp/types.hpp>

#include "test/io_runner.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <nlohmann/json.hpp>

#include <boost/asio.hpp>

#include <algorithm>
#include <random>
#include <thread>
#include <vector>

using nlohmann::json;
using namespace nsl;

TEST_CASE("reading and writing to UDP streams") {
  auto io = boost::asio::io_context{};

  auto mtx                 = std::mutex{};
  auto recv_thread_running = std::condition_variable{};

  constexpr auto test_port = udp::port_number{40000};

  SECTION("blocking streams") {
    auto udp_in  = udp::istream{io, test_port};
    auto udp_out = udp::ostream{"localhost", test_port};

    SECTION("writing and reading JSON") {
      auto recv_json = json{};
      auto sent_json = json::parse(R"({"bool_field": true, "int_field": 12345, "string_field": "ahoy there!"})");

      auto lock          = std::unique_lock<std::mutex>{mtx};
      auto data_received = std::async([&]() {
        auto lock = std::unique_lock<std::mutex>{};
        recv_thread_running.notify_all();

        udp_in >> recv_json;
      });

      recv_thread_running.wait(lock);

      udp_out << sent_json << std::endl;

      data_received.wait();

      REQUIRE(sent_json == recv_json);
    }

    SECTION("writing and reading 1024 bytes") {
      auto sent_bytes  = std::vector<std::byte>{};
      auto rng         = std::mt19937{345234};  // arbitrary seed.
      auto random_byte = std::uniform_int<>{0x00, 0xFF};
      std::generate_n(std::back_inserter(sent_bytes), 1024, [&]() { return static_cast<std::byte>(random_byte(rng)); });
      auto recv_bytes  = std::vector<std::byte>(sent_bytes.size(), std::byte{0});

      auto lock          = std::unique_lock<std::mutex>{mtx};
      auto data_received = std::async([&]() {
        auto lock = std::unique_lock<std::mutex>{};
        recv_thread_running.notify_all();

        udp_in >> recv_bytes;
      });

      recv_thread_running.wait(lock);
      udp_out << sent_bytes << udp::flush;

      data_received.wait();

      REQUIRE(std::ranges::equal(sent_bytes,recv_bytes));
    }
  }
}