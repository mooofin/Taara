#include "ribo/Lowering.h"

namespace ribo::lowering {
Plan select(std::string_view topology, std::optional<long long> depth, std::optional<long long> width) {
  if (const auto *descriptor = topology::find(topology)) return select(descriptor->kind, depth, width);
  return select(topology::Kind::bounded_crossing, depth, width);
}

Plan select(topology::Kind kind, std::optional<long long> depth, std::optional<long long> width) {
  const auto name = topology::describe(kind)->name;
  Plan p{.crossingDepth = depth, .stateWidth = width};
  if (kind == topology::Kind::nested)
    p = {.recurrence = "interval", .decomposition = "interval", .stateRank = "2", .time = "O(n^3)", .space = "O(n^2)",
         .strategy = "exact-triangular-dp", .optimality = "exact", .supported = true};
  else if (kind == topology::Kind::h_type_pseudoknot)
    p = {.recurrence = "bounded-h-type", .decomposition = "canonical-tree-decomposition", .stateRank = "4", .time = "O(n^4)", .space = "O(n^4)",
         .strategy = "h_type_bounded_dp", .optimality = "restricted-exact", .supported = true};
  else if (kind == topology::Kind::kissing_hairpin)
    p = {.recurrence = "tgb-ccj", .decomposition = "shared-stem-three-stem-five-loop", .stateRank = "4", .time = "Theta(n^5)", .space = "Theta(n^4)",
         .strategy = "coupled-tgb-dp-required", .optimality = "unimplemented"};
  else if (kind == topology::Kind::general_crossing)
    p = {.recurrence = "search", .decomposition = "search-space", .stateRank = "unbounded", .time = "not-polynomially-bounded",
         .space = "implementation-dependent", .strategy = "explicit-heuristic-required",
         .optimality = "heuristic-or-search"};
  else
    p = {.recurrence = std::string(name), .decomposition = "bounded-crossing-decomposition", .stateRank = ">2", .time = "topology-dependent",
         .space = "topology-dependent", .strategy = "specialized-dp-required", .optimality = "unimplemented"};
  p.crossingDepth = depth;
  p.stateWidth = width;
  p.sparsifiable = kind == topology::Kind::nested;
  return p;
}
}
