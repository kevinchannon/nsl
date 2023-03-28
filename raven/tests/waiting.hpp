#pragma once

#include <chrono>
#include <thread>

namespace test {

template <typename Condition_T>
[[nodiscard]] bool wait_for(Condition_T condition, std::chrono::milliseconds timeout) {
  using namespace std::chrono_literals;

  const auto start = std::chrono::high_resolution_clock::now();
  while ((std::chrono::high_resolution_clock::now() - start) < timeout) {
    if (condition()) {
      return true;
    }

    std::this_thread::sleep_for(1ms);
  }

  return false;
}

}  // namespace test
