#pragma once

#include <optional>
#include <string>
#include <string_view>
#include "ribo/Topology.h"

namespace ribo::lowering {
struct Plan {
  std::string recurrence, decomposition, stateRank, time, space, strategy, optimality;
  std::optional<long long> crossingDepth, stateWidth;
  bool supported = false, sparsifiable = false;
};
[[nodiscard]] Plan select(std::string_view topology, std::optional<long long> crossingDepth = {},
                          std::optional<long long> stateWidth = {});
[[nodiscard]] Plan select(topology::Kind topology, std::optional<long long> crossingDepth = {},
                          std::optional<long long> stateWidth = {});
}
