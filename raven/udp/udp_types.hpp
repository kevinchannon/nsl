#pragma once

#include <cstdint>

namespace raven::udp {

using port_number = std::uint16_t;

constexpr auto any_port = port_number{0};

}
