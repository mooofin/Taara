#pragma once

#include <array>
#include <optional>
#include <string_view>

namespace ribo::topology {
enum class Kind { nested, h_type_pseudoknot, kissing_hairpin, bounded_crossing, general_crossing };

struct Descriptor {
  Kind kind;
  std::string_view name, recurrence, strategy, optimality;
  bool supported;
};

inline constexpr std::array descriptors{
    Descriptor{Kind::nested, "nested", "interval", "exact-triangular-dp", "exact", true},
    Descriptor{Kind::h_type_pseudoknot, "h_type_pseudoknot", "bounded-h-type", "h_type_bounded_dp", "restricted-exact", true},
    Descriptor{Kind::kissing_hairpin, "kissing_hairpin", "tgb-ccj", "coupled-tgb-dp-required", "unimplemented", false},
    Descriptor{Kind::bounded_crossing, "bounded_crossing", "bounded-crossing", "specialized-dp-required", "unimplemented", false},
    Descriptor{Kind::general_crossing, "general_crossing", "search", "explicit-heuristic-required", "heuristic-or-search", false},
};

[[nodiscard]] constexpr const Descriptor *find(std::string_view name) {
  for (const auto &descriptor : descriptors) if (descriptor.name == name) return &descriptor;
  return nullptr;
}
[[nodiscard]] constexpr const Descriptor *describe(Kind kind) {
  for (const auto &descriptor : descriptors) if (descriptor.kind == kind) return &descriptor;
  return nullptr;
}
}
