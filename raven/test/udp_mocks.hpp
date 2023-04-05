#pragma once

#include "udp_emitter.hpp"
#include "udp_receiver.hpp"

#include <functional>

namespace raven::test::udp {
class emitter_mock : public ::raven::udp::emitter {
 public:
  [[nodiscard]] size_t send(std::istream& data) override { return send_fn(data); }

  std::function<size_t(std::istream&)> send_fn{};
};

class receiver_mock : public ::raven::udp::receiver {
 public:
  [[nodiscard]] ::raven::udp::port_number connected_port() const override { return 0; }
  void stop() override {};
};

}  // namespace raven::test::udp
