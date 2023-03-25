#include "udp_input_socket.hpp"

#include <Ws2tcpip.h>
#include <winsock2.h>

#include <format>
#include <stdexcept>

namespace raven {

namespace detail {

  class windows_udp_input_socket : public udp_input_socket {
   public:
    explicit windows_udp_input_socket(std::uint16_t port) {
      _socket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
      if (_socket == INVALID_SOCKET) {
        throw std::runtime_error{std::format("input socket failed: {}", WSAGetLastError())};
      }

      auto recv_address            = sockaddr_in{};
      recv_address.sin_family      = AF_INET;
      recv_address.sin_port        = htons(port);
      recv_address.sin_addr.s_addr = INADDR_ANY;

      const auto result = ::bind(_socket, reinterpret_cast<sockaddr*>(&recv_address), sizeof(recv_address));
      if (SOCKET_ERROR == result) {
        throw std::runtime_error{std::format("input socket failed to bind: {}", WSAGetLastError())};
      }
    }

    ~windows_udp_input_socket() { ::closesocket(_socket);
    }

    [[nodiscard]] int recv(std::ostream& data) override {
      char recvBuf[1024];
      int recvBufLen = 1024;
      sockaddr_in senderAddr;
      int senderAddrLen = sizeof(senderAddr);
      const auto result           = ::recvfrom(_socket, recvBuf, recvBufLen, 0, (sockaddr*)&senderAddr, &senderAddrLen);
      if (SOCKET_ERROR == result) {
        throw std::runtime_error{std::format("input socket failed to receive: {}", WSAGetLastError())};
      }

      std::copy_n(recvBuf, recvBufLen, std::ostreambuf_iterator<char>{data});

      return result;
    }

   private:
    SOCKET _socket{};
  };

}  // namespace detail

udp_input_socket::ptr udp_input_socket::create(std::uint16_t port) {
  return std::make_unique<detail::windows_udp_input_socket>(port);
}

}  // namespace raven
