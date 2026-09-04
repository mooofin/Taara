#include "ribo/Constraints.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <cctype>
#include <vector>

namespace ribo::runtime {

bool validShapeReactivities(const std::vector<double> &reactivities, std::string *error) {
  for (std::size_t i = 0; i < reactivities.size(); ++i) {
    if (!std::isfinite(reactivities[i])) {
      if (error) *error = "SHAPE reactivity at index " + std::to_string(i) + " is not finite";
      return false;
    }
    if (reactivities[i] < 0.0) {
      if (error) *error = "SHAPE reactivity at index " + std::to_string(i) + " is negative";
      return false;
    }
  }
  return true;
}

std::vector<double> shapePenalties(const std::vector<double> &reactivities, ShapeCalibration calibration) {
  std::vector<double> penalties;
  penalties.reserve(reactivities.size());
  for (double value : reactivities) penalties.push_back(calibration.slope * std::log1p(value) + calibration.intercept);
  return penalties;
}

FoldResult foldNestedWithShape(const std::string &sequence, int minLoop, int canonicalScore,
                               const std::vector<double> &reactivities, ShapeCalibration calibration) {
  if (reactivities.size() != sequence.size() || !validShapeReactivities(reactivities) || !std::isfinite(calibration.slope) || !std::isfinite(calibration.intercept) || calibration.slope <= 0.0)
    return {0, std::string(sequence.size(), '.')};
  const auto penalties = shapePenalties(reactivities, calibration);
  const int n = static_cast<int>(sequence.size());
  std::vector<std::vector<int>> dp(n, std::vector<int>(n));
  auto pair = [](char a, char b) {
    a = static_cast<char>(std::toupper(static_cast<unsigned char>(a)));
    b = static_cast<char>(std::toupper(static_cast<unsigned char>(b)));
    return (a == 'A' && b == 'U') || (a == 'U' && b == 'A') || (a == 'C' && b == 'G') || (a == 'G' && b == 'C');
  };
  for (int width = 1; width < n; ++width)
    for (int i = 0, j = width; j < n; ++i, ++j) {
      dp[i][j] = std::max(dp[i + 1][j], dp[i][j - 1]);
      if (j - i > minLoop && pair(sequence[i], sequence[j])) {
        int reward = canonicalScore - static_cast<int>(penalties[i] + penalties[j]);
        dp[i][j] = std::max(dp[i][j], dp[i + 1][j - 1] + reward);
      }
      for (int k = i + 1; k < j; ++k) dp[i][j] = std::max(dp[i][j], dp[i][k] + dp[k + 1][j]);
    }
  std::string structure(n, '.');
  std::function<void(int, int)> traceback = [&](int i, int j) {
    if (i >= j) return;
    if (dp[i][j] == dp[i + 1][j]) {
      traceback(i + 1, j);
      return;
    }
    if (dp[i][j] == dp[i][j - 1]) {
      traceback(i, j - 1);
      return;
    }
    if (j - i > minLoop && pair(sequence[i], sequence[j])) {
      int reward = canonicalScore - static_cast<int>(penalties[i] + penalties[j]);
      if (dp[i][j] == dp[i + 1][j - 1] + reward) {
        structure[i] = '(';
        structure[j] = ')';
        traceback(i + 1, j - 1);
        return;
      }
    }
    for (int k = i + 1; k < j; ++k)
      if (dp[i][j] == dp[i][k] + dp[k + 1][j]) {
        traceback(i, k);
        traceback(k + 1, j);
        return;
      }
  };
  if (n) traceback(0, n - 1);
  return {n ? dp[0][n - 1] : 0, structure};
}

FoldResult foldHTypeWithShape(const std::string &sequence, int g1, int g2, int g3, int h1, int h2, int score,
                              const std::vector<double> &reactivities, ShapeCalibration calibration, int wobbleScore) {
  const int n = static_cast<int>(sequence.size());
  if (h1 < 1 || h2 < 1 || g1 < 0 || g2 < 0 || g3 < 0 || score < 0 ||
      reactivities.size() != sequence.size() || !validShapeReactivities(reactivities) || !std::isfinite(calibration.slope) || !std::isfinite(calibration.intercept) || calibration.slope <= 0.0) return {0, std::string(n, '.')};
  const auto penalty = shapePenalties(reactivities, calibration);
  const auto pair = [wobbleScore](char a, char b) {
    a = static_cast<char>(std::toupper(static_cast<unsigned char>(a))); b = static_cast<char>(std::toupper(static_cast<unsigned char>(b)));
    if ((a == 'A' && b == 'U') || (a == 'U' && b == 'A') || (a == 'C' && b == 'G') || (a == 'G' && b == 'C')) return 1;
    return wobbleScore && ((a == 'G' && b == 'U') || (a == 'U' && b == 'G')) ? 2 : 0;
  };
  double best = 0.; int ba = -1, bc = -1, bb = -1, bd = -1;
  for (int a = 0; a + h1 <= n; ++a) for (int c = a + h1 + g1; c + h2 <= n; ++c)
    for (int b = c + h2 + g2; b + h1 <= n; ++b) for (int d = b + h1 + g3; d + h2 <= n; ++d) {
      double current = 0.; bool valid = true;
      for (int k = 0; k < h1; ++k) { const int i = a + k, j = b + h1 - 1 - k; const int p = pair(sequence[i], sequence[j]); if (!p) { valid = false; break; } current += (p == 2 ? wobbleScore : score) - penalty[i] - penalty[j]; }
      for (int k = 0; valid && k < h2; ++k) { const int i = c + k, j = d + h2 - 1 - k; const int p = pair(sequence[i], sequence[j]); if (!p) { valid = false; break; } current += (p == 2 ? wobbleScore : score) - penalty[i] - penalty[j]; }
      if (valid && current > best) best = current, ba = a, bc = c, bb = b, bd = d;
    }
  std::string structure(n, '.');
  if (ba >= 0) { for (int k = 0; k < h1; ++k) structure[ba + k] = '(', structure[bb + h1 - 1 - k] = ')'; for (int k = 0; k < h2; ++k) structure[bc + k] = '[', structure[bd + h2 - 1 - k] = ']'; }
  return {static_cast<int>(std::lround(best)), structure};
}

}
