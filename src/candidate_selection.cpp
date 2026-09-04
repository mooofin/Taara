#include "ribo/CandidateSelection.h"
#include <algorithm>
#include <ranges>
#include <limits>
#include <unordered_set>
namespace ribo::selection {
Result selectMaximumWeight(const std::vector<Candidate> &candidates) {
  Result best; std::vector<std::size_t> current; std::unordered_set<int> used;
  std::vector<std::int64_t> upper(candidates.size() + 1);
  for (std::size_t i = candidates.size(); i-- > 0;) upper[i] = upper[i + 1] > std::numeric_limits<std::int64_t>::max() - std::max<std::int64_t>(0, candidates[i].weight) ? std::numeric_limits<std::int64_t>::max() : upper[i + 1] + std::max<std::int64_t>(0, candidates[i].weight);
  const auto search = [&](auto &&self, std::size_t i, std::int64_t weight) -> void {
    if (i == candidates.size()) { if (weight > best.weight) best = {weight, current}; return; }
    if (upper[i] <= best.weight - weight) return;
    if (candidates[i].weight < 0 || std::ranges::any_of(candidates[i].occupied, [&](int p) { return used.contains(p); })) { self(self, i + 1, weight); return; }
    for (const int p : candidates[i].occupied) used.insert(p); current.push_back(i);
    const auto next = candidates[i].weight > std::numeric_limits<std::int64_t>::max() - weight
                          ? std::numeric_limits<std::int64_t>::max() : weight + candidates[i].weight;
    self(self, i + 1, next); current.pop_back();
    for (const int p : candidates[i].occupied) used.erase(p);
    self(self, i + 1, weight);
  };
  search(search, 0, 0); return best;
}
}
