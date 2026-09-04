#pragma once

#include <concepts>
#include <expected>
#include <limits>
#include <string>
#include <type_traits>

namespace ribo::checked {
template <std::integral To, std::integral From>
[[nodiscard]] constexpr std::expected<To, std::string> narrow(From value) {
  if constexpr (std::is_signed_v<From> == std::is_signed_v<To>) {
    if (value < static_cast<From>(std::numeric_limits<To>::min()) ||
        value > static_cast<From>(std::numeric_limits<To>::max()))
      return std::unexpected{"integer value is outside the target range"};
  } else if constexpr (std::is_signed_v<From>) {
    if (value < 0 || static_cast<std::make_unsigned_t<From>>(value) > std::numeric_limits<To>::max())
      return std::unexpected{"integer value is outside the target range"};
  } else if (value > static_cast<std::make_unsigned_t<To>>(std::numeric_limits<To>::max())) {
    return std::unexpected{"integer value is outside the target range"};
  }
  return static_cast<To>(value);
}
}
