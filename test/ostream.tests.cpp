#include "framework.h"

#include <nsl/udp/istream.hpp>
#include <nsl/udp/ostream.hpp>
#include <nsl/udp/types.hpp>

#include "test/io_runner.hpp"
#include "test/udp_receiver.hpp"
#include "test/waiting.hpp"

#include <fmt/format.h>
#include <boost/asio.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

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

TEST_CASE("blocking UDP ostream operations tests") {
  using namespace nsl;

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
                                  size_t{0x800000});

  SECTION(fmt::format("sends and receives {} bytes of data", data_size)) {
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
    auto input_socket = nsl::test::udp::receiver{io, test_port, std::move(process_data), static_cast<int>(data_size)};
    auto io_runner    = test::io_runner{io};

    auto udp_stream = udp::ostream{io, "localhost", test_port};
    udp_stream << data << udp::flush;

    if (not test::wait_for([&]() { return received_size.load() == data_size; }, 3s)) {
      std::cerr << "Received " << received_size.load() << " bytes. Expected " << data_size << std::endl;
      REQUIRE(false);
    }

    REQUIRE(data == received_data);
  }
}

TEST_CASE("non-blocking UDP ostream operations tests", "[.][WIP]") {
  using namespace nsl;

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
                                  size_t{0x800000});

  SECTION(fmt::format("sends and receives {} bytes of data", data_size)) {
    auto data = std::string(data_size, '\0');
    std::generate_n(data.begin(), data.size(), [&rng]() {
      return static_cast<char>(std::uniform_int_distribution<std::int32_t>{0x30, 0x39}(rng));
    });

    auto buffer = std::vector<char>{};

    auto recv_mtx      = std::mutex{};
    auto data_recv     = std::condition_variable{};
    auto received_data = std::string(data_size, '\0');
    auto write_pos     = received_data.begin();
    auto received_size = std::atomic_size_t{0};
    auto recv_lock     = std::unique_lock{recv_mtx};
    auto process_data  = [&](auto& recv_buf, auto byte_count) {
      std::copy_n(std::istreambuf_iterator<char>{recv_buf}, byte_count, write_pos);
      std::advance(write_pos, byte_count);
      received_size += byte_count;

      if (received_size.load() == data_size) {
        { auto _ = std::unique_lock{recv_mtx}; }
        data_recv.notify_all();
      }
    };

    buffer.resize(4096);
    auto input_socket = nsl::test::udp::receiver{io, test_port, std::move(process_data), static_cast<int>(data_size)};
    auto io_runner    = test::io_runner{io};

    auto remote = udp::ostream{io, "localhost", test_port};

    auto send_mtx           = std::mutex{};
    auto data_sent          = std::condition_variable{};
    auto sent_bytes         = std::atomic_size_t{0};
    auto lock               = std::unique_lock{send_mtx};
    const auto send_handler = [&](size_t n) {
      sent_bytes += n;
      { auto _ = std::unique_lock{send_mtx}; }
      data_sent.notify_all();
    };

    remote << std::pair{data, send_handler};

    const auto data_sent_in_time = data_sent.wait_for(lock, 3s, [&]() { return sent_bytes.load() == data.size(); });
    REQUIRE(data_sent_in_time);

    if (not data_recv.wait_for(recv_lock, 3s, [&]() { return received_size.load() == data_size; })) {
      std::cerr << "Received " << received_size.load() << " bytes. Expected " << data_size << std::endl;
      REQUIRE(false);
    }

    REQUIRE(data == received_data);
  }
}
