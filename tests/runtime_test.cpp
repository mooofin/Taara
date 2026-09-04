#include "ribo/Constraints.h"
#include "ribo/Runtime.h"
#include "ribo/Sequence.h"
#include "ribo/CandidateSelection.h"
#include "ribo/Energy.h"
#include <cmath>
#include <cstdint>
#include <limits>
#include <iostream>

int main() {
  auto empty = ribo::runtime::foldNested("", 3, 1);
  auto pair = ribo::runtime::foldNested("AU", 0, 1);
  auto wobble = ribo::runtime::foldNested("GU", 0, 1, 2);
  auto disabledWobble = ribo::runtime::foldNested("GU", 0, 1, 0);
  auto lowercase = ribo::runtime::foldNested("au", 0, 1);
  auto invalid = ribo::runtime::foldNested("AU", -1, 1);
  auto invalidBase = ribo::runtime::foldNested("AX", 0, 1);
  if (ribo::sequence::normalize(">sample one\n gcg\n aa u ") != "GCGAAU") return 20;
  if (ribo::sequence::normalize("  a\tc\n") != "AC") return 21;
  if (empty.score != 0 || !empty.structure.empty()) return 1;
  if (pair.score != 1 || pair.structure != "()") return 2;
  if (wobble.score != 2 || wobble.structure != "()") return 3;
  if (disabledWobble.score != 0 || disabledWobble.structure != "..") return 22;
  if (lowercase.score != 1 || lowercase.structure != "()") return 4;
  if (invalid.score != 0 || invalid.structure != "..") return 11;
  if (invalidBase.score != 0 || invalidBase.structure != "..") return 12;
  std::string error;
  if (!ribo::runtime::validShapeReactivities({0.0, 1.2, 2.4}, &error)) return 5;
  if (ribo::runtime::validShapeReactivities({0.0, -1.0}, &error)) return 6;
  auto penalties = ribo::runtime::shapePenalties({0.0, 1.0});
  if (penalties.size() != 2 || std::fabs(penalties[0] + 0.6) > 1e-9 || std::fabs(penalties[1] - (1.8 * std::log1p(1.0) - 0.6)) > 1e-9) return 7;
  auto constrained = ribo::runtime::foldNestedWithShape("AU", 0, 1, {10.0, 10.0});
  if (constrained.score >= pair.score) return 8;
  auto tolerated = ribo::runtime::foldNestedWithShape("AU", 0, 1, {0.4, 0.4});
  if (tolerated.structure != "()") return 9;
  auto constrainedLowercase = ribo::runtime::foldNestedWithShape("au", 0, 1, {0.4, 0.4});
  if (constrainedLowercase.structure != "()") return 10;
  auto hType = ribo::runtime::foldHType("GCAGCAAGCAGC", 1, 2, 1);
  if (hType.score != 4 || hType.structure.find('[') == std::string::npos) return 13;
  auto hTooTight = ribo::runtime::foldHType("GCAGCAAGCAGC", 4, 2, 1);
  if (hTooTight.score != 0 || hTooTight.structure != std::string(12, '.')) return 14;
  auto hInvalid = ribo::runtime::foldHType("GCAGXAAGCAGC", 1, 2, 1);
  if (hInvalid.score != 0 || hInvalid.structure != std::string(12, '.')) return 15;
  auto hLongStem = ribo::runtime::foldHType("GCAGCAAGCAGC", 1, 3, 1);
  if (hLongStem.score != 0 || hLongStem.structure != std::string(12, '.')) return 16;
  auto hUnequal = ribo::runtime::foldHType("GCAGCAAGCAGC", 1, 2, 1, 1);
  if (hUnequal.score <= 0 || hUnequal.structure.find('[') == std::string::npos) return 17;
  auto hBoundary = ribo::runtime::foldHType("GCGCGCGCGCGC", 0, 5, 1, 1);
  if (hBoundary.structure.size() != 12) return 18;
  auto hRoleSpecific = ribo::runtime::foldHType("GCAGCAAGCAGC", 1, 2, 1, 2, 1, 1);
  if (hRoleSpecific.score != 3 || hRoleSpecific.structure.find('[') == std::string::npos) return 19;
  auto hShape = ribo::runtime::foldHTypeWithShape("GCAGCAAGCAGC", 1, 1, 1, 2, 1, 1, std::vector<double>(12, 0.0));
  if (hShape.score != 7 || hShape.structure.find('[') == std::string::npos) return 23;
  auto hShapeBlocked = ribo::runtime::foldHTypeWithShape("GCAGCAAGCAGC", 1, 1, 1, 2, 1, 1, std::vector<double>(12, 10.0));
  if (hShapeBlocked.score != 0 || hShapeBlocked.structure != std::string(12, '.')) return 24;
  const auto hShapeWobble = ribo::runtime::foldHTypeWithShape("GAUU", 0, 0, 0, 1, 1, 1, std::vector<double>(4, 0.0), {}, 2);
  if (hShapeWobble.score != 5 || hShapeWobble.structure.find('[') == std::string::npos) return 28;
  const std::string stemSequence = "GCAGCAAGCAGC";
  if (ribo::runtime::scoreStem(stemSequence, 0, 11, 2, 1) != 2 ||
      ribo::runtime::scoreStem(stemSequence, 0, 11, 2, 1, 0) != 2 ||
      ribo::runtime::scoreStem("GAUU", 0, 3, 1, 1, 2) != 2 ||
      ribo::runtime::scoreStem("GAUU", 0, 3, 1, 1) != 0) return 30;
  const auto kissScore = ribo::runtime::scoreKissingCandidate(stemSequence,
      {{{0, 11, 2}, {1, 10, 1}, {3, 8, 2}}}, 2, 1);
  const auto kissWobbleScore = ribo::runtime::scoreKissingCandidate(stemSequence,
      {{{0, 11, 2}, {1, 10, 1}, {3, 8, 2}}}, 2, 1, 2);
  if (kissScore != 4 ||
      kissWobbleScore != 4) return 31;
  if (ribo::runtime::scoreKissingCandidate(stemSequence,
      {{{0, 11, 2}, {2, 9, 1}, {3, 8, 2}}}, 2, 1) != 0) return 32;
  if (ribo::runtime::scoreKissingCandidate(stemSequence,
      {{{0, 11, 1}, {1, 10, 1}, {0, 11, 2}}}, 3, 1) != 0) return 34;
  if (ribo::runtime::scoreKissingCandidate(stemSequence,
      {{{0, 11, 2}, {1, 10, 1}, {3, 8, 2}}}, 2, -1) != 0) return 33;
  if (ribo::runtime::foldNestedWithShape("AU", 0, 1, {0.0, 0.0}, {.slope = 0.0}).score != 0) return 29;
  std::string energyError;
  if (!ribo::energy::valid({.parameterSet = "Turner2023"}, &energyError) ||
      ribo::energy::valid({.temperatureK = 0.0, .parameterSet = "Turner2023"}, &energyError) ||
      ribo::energy::valid({.sodiumM = -0.1, .parameterSet = "Turner2023"}, &energyError) ||
      ribo::energy::valid({}, &energyError)) return 35;
  const auto selected = ribo::selection::selectMaximumWeight({{5, {1, 2}}, {7, {2, 3}}, {4, {8}}});
  if (selected.weight != 11 || selected.selected.size() != 2) return 25;
  const auto tie = ribo::selection::selectMaximumWeight({{4, {10}}, {4, {10}}, {-9, {12}}});
  if (tie.weight != 4 || tie.selected.size() != 1 || tie.selected.front() != 0) return 26;
  const auto saturated = ribo::selection::selectMaximumWeight({{std::numeric_limits<std::int64_t>::max(), {20}}, {1, {21}}});
  if (saturated.weight != std::numeric_limits<std::int64_t>::max() || saturated.selected.size() != 2) return 27;
  std::cout << "runtime nested DP checks passed\n";
  return 0;
}
