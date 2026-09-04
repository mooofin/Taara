#pragma once

#include <string_view>

namespace ribo::metrics {
struct PairMetrics {
  bool valid = true;
  bool predictedCrossing = false, referenceCrossing = false;
  int truePositive = 0, falsePositive = 0, falseNegative = 0;
  [[nodiscard]] double sensitivity() const;
  [[nodiscard]] double positivePredictiveValue() const;
  [[nodiscard]] double f1() const;
};
[[nodiscard]] PairMetrics compareStructures(std::string_view predicted, std::string_view reference);
}
