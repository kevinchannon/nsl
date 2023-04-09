#pragma once

#include <wite/io/concepts.hpp>

#include <cstdint>
#include <tuple>

namespace raven::udp {

using port_number = std::uint16_t;

constexpr auto any_port = port_number{0};

template <typename T>
concept contiguous_byte_range_like = requires(T& t) {
                                       std::ignore = wite::io::is_byte_like_v<typename std::decay_t<T>::value_type>;
                                       std::ignore = t.begin();
                                       std::ignore = t.end();
                                       std::ignore = t.size();
                                       std::ignore = t.data();
                                     };

}  // namespace raven::udp
