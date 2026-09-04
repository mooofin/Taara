#include "ribo/Metrics.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <string>

static std::string load(std::string value) {
  if (value.empty() || value.front() != '@') return value;
  std::ifstream input(value.substr(1));
  if (!input) return {};
  value.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
  value.erase(std::remove_if(value.begin(), value.end(), [](unsigned char c) { return std::isspace(c); }), value.end());
  return value;
}

int main(int argc, char **argv) {
  const bool json = argc == 4 && std::string(argv[1]) == "--json";
  if (argc != 3 && !json) {
    std::cerr << "usage: ribo-evaluate [--json] PREDICTED_DOT_BRACKET REFERENCE_DOT_BRACKET\n";
    return 2;
  }
  const auto predicted = load(argv[json ? 2 : 1]), reference = load(argv[json ? 3 : 2]);
  if (predicted.empty() || reference.empty()) { std::cerr << "error: unable to read structure input\n"; return 1; }
  const auto m = ribo::metrics::compareStructures(predicted, reference);
  if (!m.valid) { std::cerr << "error: malformed or length-mismatched dot-bracket structure\n"; return 1; }
  if (json)
    std::cout << "{\"schema\":\"ribo.metrics.v1\",\"predicted_length\":" << predicted.size()
              << ",\"reference_length\":" << reference.size() << ",\"predicted_topology\":\""
              << (m.predictedCrossing ? "crossing" : "nested") << "\",\"reference_topology\":\""
              << (m.referenceCrossing ? "crossing" : "nested") << "\",\"topology_match\":"
              << (m.predictedCrossing == m.referenceCrossing ? "true" : "false") << ",\"tp\":" << m.truePositive << ",\"fp\":" << m.falsePositive << ",\"fn\":" << m.falseNegative
              << ",\"sensitivity\":" << m.sensitivity() << ",\"ppv\":" << m.positivePredictiveValue()
              << ",\"f1\":" << m.f1() << "}\n";
  else
    std::cout << "predicted_topology=" << (m.predictedCrossing ? "crossing" : "nested")
              << " reference_topology=" << (m.referenceCrossing ? "crossing" : "nested")
              << " topology_match=" << (m.predictedCrossing == m.referenceCrossing ? "yes" : "no")
              << " tp=" << m.truePositive << " fp=" << m.falsePositive << " fn=" << m.falseNegative
              << " sensitivity=" << m.sensitivity() << " ppv=" << m.positivePredictiveValue() << " f1=" << m.f1() << "\n";
}
