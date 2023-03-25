#include "udp_output_socket.hpp"

#include <winsock2.h>
#include <Ws2tcpip.h>

#include <format>
#include <stdexcept>

namespace raven {

namespace detail {

  class windows_udp_output_socket : public udp_output_socket {
   public:
    explicit windows_udp_output_socket(std::string ip, std::uint16_t port) {
      _socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
      if (_socket == INVALID_SOCKET) {
        throw std::runtime_error{std::format("socket failed: {}", WSAGetLastError())};
      }

      _dest_addr.sin_family      = AF_INET;
      _dest_addr.sin_port        = htons(port);
      InetPton(AF_INET, ip.c_str(), &_dest_addr.sin_addr.s_addr);
    }

    [[nodiscard]] int send(std::istream&) override {
      return 0;
    }

   private:
    SOCKET _socket{};
    sockaddr_in _dest_addr{};
  };

}  // namespace detail

udp_output_socket::ptr udp_output_socket::create(std::string ip, std::uint16_t port) {
  return std::make_unique<detail::windows_udp_output_socket>(std::move(ip), port);
}

}  // namespace raven
