#include "ribo/Metrics.h"

#include <array>
#include <algorithm>
#include <set>
#include <utility>
#include <vector>

namespace ribo::metrics {
namespace {
using Pair = std::pair<int, int>;
struct Parsed { std::set<Pair> contacts; bool valid = true; };
bool crossing(const std::set<Pair> &contacts) {
  for (const auto &[a, b] : contacts)
    for (const auto &[c, d] : contacts)
      if (a < c && c < b && b < d) return true;
  return false;
}
Parsed pairs(std::string_view structure) {
  std::array<std::vector<int>, 3> opens;
  std::set<Pair> result;
  for (int i = 0; i < static_cast<int>(structure.size()); ++i) {
    const char c = structure[i];
    if (c == '(' || c == '[' || c == '{') opens[c == '[' ? 1 : c == '{' ? 2 : 0].push_back(i);
    if ((c == ')' || c == ']' || c == '}') && !opens[c == ')' ? 0 : c == ']' ? 1 : 2].empty()) {
      auto &stack = opens[c == ')' ? 0 : c == ']' ? 1 : 2];
      result.emplace(stack.back(), i);
      stack.pop_back();
    } else if (c != '(' && c != '[' && c != '{' && c != ')' && c != ']' && c != '}' && c != '.') return {.valid = false};
  }
  return {.contacts = std::move(result), .valid = opens[0].empty() && opens[1].empty() && opens[2].empty()};
}
}
double PairMetrics::sensitivity() const { return falseNegative + truePositive ? static_cast<double>(truePositive) / (truePositive + falseNegative) : 0.0; }
double PairMetrics::positivePredictiveValue() const { return falsePositive + truePositive ? static_cast<double>(truePositive) / (truePositive + falsePositive) : 0.0; }
double PairMetrics::f1() const {
  const auto recall = sensitivity(), precision = positivePredictiveValue();
  return recall + precision ? 2.0 * recall * precision / (recall + precision) : 0.0;
}
PairMetrics compareStructures(std::string_view predicted, std::string_view reference) {
  const auto p = pairs(predicted), r = pairs(reference);
  PairMetrics m;
  m.valid = p.valid && r.valid && predicted.size() == reference.size();
  if (!m.valid) return m;
  m.predictedCrossing = crossing(p.contacts);
  m.referenceCrossing = crossing(r.contacts);
  for (const auto &pair : p.contacts) r.contacts.contains(pair) ? ++m.truePositive : ++m.falsePositive;
  for (const auto &pair : r.contacts) if (!p.contacts.contains(pair)) ++m.falseNegative;
  return m;
}
}
