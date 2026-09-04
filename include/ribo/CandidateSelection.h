#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>
namespace ribo::selection {
struct Candidate { std::int64_t weight = 0; std::vector<int> occupied; };
struct Result { std::int64_t weight = 0; std::vector<std::size_t> selected; };
[[nodiscard]] Result selectMaximumWeight(const std::vector<Candidate> &candidates);
}
