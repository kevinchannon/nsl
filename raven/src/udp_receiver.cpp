#include "udp_receiver.hpp"

#include "framework.h"

#include <boost/asio.hpp>

#include <format>
#include <optional>
#include <span>
#include <stdexcept>

namespace bai = boost::asio::ip;

namespace raven::udp {

std::unique_ptr<receiver> receiver::create(boost::asio::io_context& io,
                                           port_number port,
                                           callback_t handle_data,
                                           std::optional<int> recv_buf_size) {
  return std::make_unique<receiver_impl>(io, port, std::move(handle_data), recv_buf_size);
}

}  // namespace raven::udp
