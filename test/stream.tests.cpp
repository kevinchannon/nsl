#include "framework.h"

#include <nsl/udp/stream.hpp>
#include <nsl/udp/types.hpp>

#include "test/io_runner.hpp"
#include "test/waiting.hpp"

#include <boost/asio.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <nlohmann/json.hpp>
#include <wite/collections/make_vector.hpp>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

using nlohmann::json;
using namespace nsl;
using namespace std::chrono_literals;

TEST_CASE("reading and writing to UDP streams") {
  auto io = boost::asio::io_context{};

  constexpr auto test_port = udp::port_number{40000};

  SECTION("blocking streams") {
    auto udp_in  = udp::istream{io, test_port};
    auto udp_out = udp::ostream{io, "localhost", test_port};

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

      auto rng = std::mt19937{345234};  // arbitrary seed.
      std::generate_n(std::back_inserter(sent_bytes), 1024, [&]() {
        return static_cast<std::byte>(std::uniform_int_distribution<>{0x00, 0xFF}(rng));
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

  SECTION("non-blocking streams") {
    auto mtx = std::mutex{};

    SECTION("writing and reading JSON") {
      auto sent_json = json::parse(R"({"bool_field": true, "int_field": 12345, "string_field": "ahoy there!"})");
      auto recv_json = json{};

      auto data_ready  = std::condition_variable{};
      auto lock        = std::unique_lock{mtx};
      auto handle_json = [&](auto&& is, size_t n) {
        auto str = std::string(n, '\0');
        is.read(str.data(), n);
        recv_json = json::parse(str);
        { auto _ = std::unique_lock{mtx}; }
        data_ready.notify_all();
      };

      auto udp_in = udp::istream{io, test_port};
      udp_in >> handle_json;

      auto _ = test::io_runner{io};

      auto udp_out = udp::ostream{io, "localhost", test_port};
      udp_out << sent_json << std::endl;

      data_ready.wait(lock);

      REQUIRE(sent_json == recv_json);
    }
  }

  SECTION("IO stream") {
    auto mtx = std::mutex{};

    SECTION("writing and reading JSON") {
      auto request  = json::parse(R"({"bool_field": true, "int_field": 12345, "string_field": "ahoy there!"})");
      auto response = json{};

      auto lock                 = std::unique_lock{mtx};
      auto server_listening     = std::condition_variable{};
      auto running_test_server = test::running_async([&]() {
        auto server_in = udp::istream{io, test_port + 1};

        { auto _ = std::unique_lock{mtx}; }
        server_listening.notify_all();

        auto req = json{};
        server_in >> req;

        auto server_out = udp::ostream{io, "localhost", test_port};
        server_out << (req == request ? json{{"result", 200}} : json{{"result", 400}}) << udp::flush;
      });

      server_listening.wait(lock);

      auto data_ready      = std::condition_variable{};
      auto handle_response = [&](auto&& is, size_t n) {
        auto str = std::string(n, '\0');
        is.read(str.data(), n);
        response = json::parse(str);
        { auto _ = std::unique_lock{mtx}; }
        data_ready.notify_all();
      };

      auto remote = udp::stream{io, test_port, "localhost", test_port + 1};

      remote >> handle_response;

      auto _ = test::io_runner{io};

      remote << request << std::endl;

      data_ready.wait(lock);

      REQUIRE(json{{"result", 200}} == response);
    }
  }
}