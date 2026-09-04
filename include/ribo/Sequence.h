#pragma once

#include <string>
#include <string_view>

namespace ribo::sequence {
[[nodiscard]] std::string normalize(std::string_view input);
}
