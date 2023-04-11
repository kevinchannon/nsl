#include "framework.h"

#include "udp/istream.hpp"
#include "udp/types.hpp"

#include "test/io_runner.hpp"
#include "test/udp_receiver.hpp"
#include "test/waiting.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <nlohmann/json.hpp>

#include <boost/asio.hpp>

#include <atomic>
#include <chrono>
#include <format>
#include <future>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string_view>
#include <thread>
#include <vector>

using namespace std::chrono_literals;
using namespace nlohmann;

std::pair<boost::asio::ip::udp::socket, boost::asio::ip::udp::endpoint> get_connected_socket(boost::asio::io_context& io,
                                                                                             nsl::udp::port_number port) {
  auto resolver = boost::asio::ip::udp::resolver{io};
  auto query    = boost::asio::ip::udp::resolver::query{
      boost::asio::ip::udp::v4(), "localhost", std::to_string(static_cast<std::uint32_t>(port))};
  auto endpoint = *resolver.resolve(query);

  auto socket = boost::asio::ip::udp::socket{io};
  socket.open(boost::asio::ip::udp::v4());

  return {std::move(socket), endpoint};
}

TEST_CASE("UDP istream tests") {
  auto io        = boost::asio::io_context{};
  auto test_port = nsl::udp::port_number{6078};

  SECTION("blocking receive") {
    SECTION("An int can be received via unformatted read") {
      auto udp_in = nsl::udp::istream{io, test_port};

      auto received = std::async([&]() {
        auto recv_buf = uint32_t{0};
        std::copy_n(std::istreambuf_iterator<char>{udp_in}, sizeof(recv_buf), reinterpret_cast<char*>(&recv_buf));
        return recv_buf;
      });

      auto [udp_out, recv_endpoint] = get_connected_socket(io, test_port);

      auto send_data = std::uint32_t{0xFEDCBA98};
      udp_out.send_to(boost::asio::buffer(reinterpret_cast<char*>(&send_data), sizeof(send_data)), recv_endpoint);

      REQUIRE(send_data == received.get());
    }

    SECTION("An int can be received via formatted stream extraction") {
      auto udp_in = nsl::udp::istream{io, test_port};

      auto received = std::async([&]() {
        auto recv_buf = uint32_t{0};
        udp_in >> recv_buf;
        return recv_buf;
      });

      auto [udp_out, recv_endpoint] = get_connected_socket(io, test_port);

      auto send_data = std::uint32_t{0xFEDCBA98};
      udp_out.send_to(boost::asio::buffer(std::to_string(send_data)), recv_endpoint);
      udp_out.send_to(boost::asio::buffer("\n"), recv_endpoint);

      REQUIRE(send_data == received.get());
    }
  }

  SECTION("asynchronous receive") {
    auto mtx           = std::mutex{};
    auto data_received = std::condition_variable{};

    SECTION("A string can be received via formatted stream extraction") {
      auto udp_in = nsl::udp::istream{io, test_port};

      auto recv_data       = std::string{};
      auto receive_a_value = [&](auto&& is, size_t n) {
        recv_data.resize(n);
        is.read(recv_data.data(), n);

        auto lock = std::unique_lock{mtx};
        data_received.notify_all();
      };

      udp_in >> receive_a_value;

      auto _ = nsl::test::io_runner{io};

      auto [udp_out, recv_endpoint] = get_connected_socket(io, test_port);

      auto send_data = std::string{"hello, Async UDP Recv!\n"};

      auto lock = std::unique_lock{mtx};
      udp_out.send_to(boost::asio::buffer(send_data), recv_endpoint);

      data_received.wait(lock);

      REQUIRE(send_data == recv_data);
    }

    SECTION("several strings can be received via formatted stream extraction") {
      auto udp_in = nsl::udp::istream{io, test_port};

      auto recv_data       = std::string{};
      auto receive_a_value = [&](auto&& is, size_t n) {
        recv_data.resize(n);
        is.read(recv_data.data(), n);

        auto lock = std::unique_lock{mtx};
        data_received.notify_all();
      };

      udp_in >> receive_a_value;

      auto _ = nsl::test::io_runner{io};

      auto [udp_out, recv_endpoint] = get_connected_socket(io, test_port);

      for (auto i = 0u; i < 10; ++i) {
        auto send_data = std::format("hello, Async UDP Recv! {}\n", i);

        std::unique_lock lock{mtx};
        udp_out.send_to(boost::asio::buffer(send_data), recv_endpoint);

        data_received.wait(lock);

        REQUIRE(send_data == recv_data);
      }
    }

    SECTION("trying to do a synchronous read while an async one is in progress fails") {
      auto udp_in = nsl::udp::istream{io, test_port};

      auto receive_a_value = [&](auto&&, size_t) {
        // Do nothing.
      };

      udp_in >> receive_a_value;

      auto io_runner = std::optional<nsl::test::io_runner>{io};

      auto dummy = std::string{"foo"};
      udp_in >> dummy;

      REQUIRE(udp_in.fail());

      SECTION("and cancelling the async read allows sync read to succeed") {
        udp_in.cancel_async_recv();

        auto recv_value = nsl::test::running_async([&]() {
          auto recv_str = std::string{};
          udp_in >> recv_str;
          return recv_str;
        });

        auto [udp_out, recv_endpoint] = get_connected_socket(io, test_port);

        udp_out.send_to(boost::asio::buffer("hello\n"), recv_endpoint);

        REQUIRE_FALSE(udp_in.fail());
        REQUIRE("hello" == recv_value.get());
      }
    }

    SECTION("trying to do an async read while a synchronous one is in progress fails") {
      auto udp_in = nsl::udp::istream{io, test_port};

      auto recv_value = nsl::test::running_async([&]() {
        auto recv_str = std::string{};
        udp_in >> recv_str;
        return recv_str;
      });

      std::this_thread::sleep_for(10ms);

      udp_in >> [](auto&&, size_t) {};

      REQUIRE(udp_in.fail());

      SECTION("after cancelling the sync read, an async read is allowed") {
        udp_in.cancel_sync_recv();

        auto recv_data       = std::string{};
        auto receive_a_value = [&](auto&& is, size_t n) {
          recv_data.resize(n);
          is.read(recv_data.data(), n);

          auto lock = std::unique_lock{mtx};
          data_received.notify_all();
        };

        udp_in >> receive_a_value;

        auto _ = nsl::test::io_runner{io};

        auto [udp_out, recv_endpoint] = get_connected_socket(io, test_port);

        auto send_data = std::string{"hello!\n"};

        auto lock = std::unique_lock{mtx};
        udp_out.send_to(boost::asio::buffer(send_data), recv_endpoint);

        data_received.wait(lock);

        REQUIRE(send_data == recv_data);
      }
    }
  }
}
