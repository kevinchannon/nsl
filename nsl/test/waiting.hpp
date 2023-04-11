#pragma once

#include <chrono>
#include <thread>
#include <type_traits>

namespace nsl::test {

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

template<typename Fn_T>
[[nodiscard]] auto running_async(Fn_T&& fn) {
  auto mtx = std::mutex{};
  auto thread_running = std::condition_variable{};

  auto lock   = std::unique_lock{mtx};
  auto result = std::async(std::launch::async, [&]() {
    { auto _ = std::unique_lock{mtx}; }
    thread_running.notify_all();
    
    if constexpr (std::is_same_v<void, decltype(std::declval<std::decay_t<Fn_T>>()())>) {
      fn();
    } else {
      return fn();
    }
  });

  thread_running.wait(lock);

  return std::move(result);
}

}  // namespace test
