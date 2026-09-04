#include "ribo/Lowering.h"

#include <iostream>

int main() {
  using ribo::lowering::select;
  const auto nested = select("nested");
  if (!nested.supported || !nested.sparsifiable || nested.recurrence != "interval" || nested.stateRank != "2" || nested.optimality != "exact") return 1;
  const auto bounded = select("bounded_crossing", 2, 4);
  if (bounded.supported || !bounded.crossingDepth || !bounded.stateWidth || *bounded.crossingDepth != 2 || *bounded.stateWidth != 4) return 2;
  const auto general = select("general_crossing");
  if (general.supported || general.sparsifiable || general.strategy != "explicit-heuristic-required" || general.optimality != "heuristic-or-search") return 3;
  std::cout << "lowering policy checks passed\n";
}
