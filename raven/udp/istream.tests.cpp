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

std::pair<boost::asio::ip::udp::socket, boost::asio::ip::udp::endpoint> get_connected_socket(boost::asio::io_context& io, raven::udp::port_number port) {
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
  auto test_port = raven::udp::port_number{6078};

  SECTION("blocking receive") {
    SECTION("An int can be received via unformatted read") {
      auto udp_in   = raven::udp::istream{io, test_port};

      auto received = std::async([&]() {
        auto recv_buf = uint32_t{0};
        std::copy_n(std::istreambuf_iterator<char>{udp_in}, sizeof(recv_buf), reinterpret_cast<char*>(&recv_buf));
        return recv_buf;
      });

      auto [udp_out, recv_endpoint] = get_connected_socket(io, test_port);
      
      auto send_data = std::uint32_t{0xFEDCBA98};
      udp_out.send_to(boost::asio::buffer(reinterpret_cast<char*>(& send_data), sizeof(send_data)), recv_endpoint);

      REQUIRE(send_data == received.get());
    }

    SECTION("An int can be received via formatted stream extraction") {
      auto udp_in = raven::udp::istream{io, test_port};

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
}
