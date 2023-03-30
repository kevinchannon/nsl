#pragma once

#include "udp_emitter.hpp"

#include <functional>

namespace raven::test::udp {
class emitter_mock : public ::raven::udp::emitter {
 public:
  [[nodiscard]] size_t send(std::istream& data) override { return send_fn(data); }

  std::function<size_t(std::istream&)> send_fn{};
};
}  // namespace raven::test::udp
