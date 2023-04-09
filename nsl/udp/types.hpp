#pragma once

#include <wite/io/concepts.hpp>

#include <cstdint>
#include <istream>
#include <tuple>
#include <type_traits>

namespace nsl::udp {

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

namespace detail {

  // Trick from https://stackoverflow.com/a/41272560
  template <typename T>
  std::add_lvalue_reference_t<T> make_lval();
}

template <typename T>
concept async_recv_fn_like = requires(T& t) { t(detail::make_lval<std::istream>(), size_t{0}); };

}  // namespace nsl::udp
