#include <catch2/catch_session.hpp>

#include <winsock2.h>

#include <format>
#include <stdexcept>

struct WSAJanitor {
  WSAJanitor() {
    auto wsaData = WSADATA{};
    int iResult  = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
      throw std::runtime_error{std::format("WSAStartup failed: {}", iResult)};
    }
  }

  ~WSAJanitor() { WSACleanup(); }
};

int main(int argc, char** argv) {
  const auto _ = WSAJanitor{};
  int result = Catch::Session().run(argc, argv);

  return result;
}