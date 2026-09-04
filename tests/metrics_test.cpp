#include "ribo/Metrics.h"

#include <cmath>
#include <iostream>

int main() {
  const auto m = ribo::metrics::compareStructures("([).]", "([).]");
  if (m.truePositive != 2 || m.falsePositive || m.falseNegative || !m.predictedCrossing || !m.referenceCrossing || m.sensitivity() != 1.0 || m.positivePredictiveValue() != 1.0 || m.f1() != 1.0) return 1;
  const auto partial = ribo::metrics::compareStructures("()()", "()..");
  if (partial.truePositive != 1 || partial.falsePositive != 1 || partial.falseNegative || partial.sensitivity() != 1.0 || std::fabs(partial.f1() - 2.0 / 3.0) > 1e-9) return 2;
  if (ribo::metrics::compareStructures("([)", "...").valid) return 3;
  if (ribo::metrics::compareStructures("()", "()..").valid) return 4;
  const auto mismatch = ribo::metrics::compareStructures("([).]", "(...)");
  if (!mismatch.valid || !mismatch.predictedCrossing || mismatch.referenceCrossing) return 5;
  const auto kissing = ribo::metrics::compareStructures("([{.}])", "([{.}])");
  if (!kissing.valid || kissing.truePositive != 3 || kissing.f1() != 1.0) return 6;
  std::cout << "structure metrics checks passed\n";
}
