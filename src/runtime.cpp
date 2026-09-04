#include "ribo/Runtime.h"
#include "ribo/Checked.h"

#include <algorithm>
#include <cctype>
#include <functional>
#include <limits>
#include <ranges>
#include <vector>

namespace ribo::runtime {
namespace {

std::int64_t saturatedAdd(std::int64_t a, int b) {
  if (b > 0 && a > std::numeric_limits<std::int64_t>::max() - b) return std::numeric_limits<std::int64_t>::max();
  return a + b;
}
std::int64_t saturatedAdd(std::int64_t a, std::int64_t b) {
  if (b > 0 && a > std::numeric_limits<std::int64_t>::max() - b) return std::numeric_limits<std::int64_t>::max();
  return a + b;
}

int pairScore(char a, char b, int canonical, int wobble) {
  a = static_cast<char>(std::toupper(static_cast<unsigned char>(a)));
  b = static_cast<char>(std::toupper(static_cast<unsigned char>(b)));
  if ((a == 'A' && b == 'U') || (a == 'U' && b == 'A') || (a == 'C' && b == 'G') || (a == 'G' && b == 'C'))
    return canonical;
  if ((a == 'G' && b == 'U') || (a == 'U' && b == 'G')) return wobble;
  return 0;
}
bool validSequence(const std::string &sequence) {
  return std::ranges::all_of(sequence, [](unsigned char c) {
    c = static_cast<unsigned char>(std::toupper(c));
    return c == 'A' || c == 'C' || c == 'G' || c == 'U';
  });
}

}

std::int64_t scoreStem(std::span<const char> sequence, int start, int end, int length,
                       int canonicalScore, int wobbleScore) {
  if (start < 0 || end < 0 || length < 1 || canonicalScore < 0 || wobbleScore < 0 ||
      end >= static_cast<int>(sequence.size()) || end <= start ||
      static_cast<std::int64_t>(length) * 2 > static_cast<std::int64_t>(end) - start + 1)
    return 0;
  std::int64_t total = 0;
  for (int k = 0; k < length; ++k) total = saturatedAdd(total, pairScore(sequence[start + k], sequence[end - k], canonicalScore, wobbleScore));
  return total;
}

std::int64_t scoreKissingCandidate(std::span<const char> sequence, std::array<KissingStem, 3> stems,
                                   int sharedStem, int canonicalScore, int wobbleScore) {
  if (sharedStem < 1 || sharedStem > 3 || canonicalScore < 0 || wobbleScore < 0) return 0;
  std::vector<std::pair<int, int>> occupied(sequence.size(), {-1, -1});
  const auto &shared = stems[sharedStem - 1];
  const auto containsPair = [](const KissingStem &stem, int i, int j) {
    return stem.start <= i && i < stem.start + stem.length && j == stem.end - (i - stem.start);
  };
  const auto containsWholeStem = [&](const KissingStem &candidate) {
    for (int k = 0; k < shared.length; ++k)
      if (!containsPair(candidate, shared.start + k, shared.end - k)) return false;
    return true;
  };
  if (!containsWholeStem(stems[(sharedStem) % 3]) && !containsWholeStem(stems[(sharedStem + 1) % 3])) return 0;
  std::int64_t total = 0;
  for (int s = 0; s < 3; ++s) {
    const auto &[start, end, length] = stems[s];
    if (start < 0 || end < 0 || length < 1 || end <= start || end >= static_cast<int>(sequence.size()) ||
        static_cast<std::int64_t>(length) * 2 > static_cast<std::int64_t>(end) - start + 1) return 0;
    for (int k = 0; k < length; ++k) {
      const auto i = start + k, j = end - k;
      const std::pair<int, int> pair{i, j};
      if ((occupied[i].first >= 0 && occupied[i] != pair) || (occupied[j].first >= 0 && occupied[j] != pair)) return 0;
      occupied[i] = occupied[j] = pair;
      if (!containsPair(shared, i, j)) total = saturatedAdd(total, pairScore(sequence[i], sequence[j], canonicalScore, wobbleScore));
    }
  }
  total = saturatedAdd(total, scoreStem(sequence, shared.start, shared.end, shared.length, canonicalScore, wobbleScore));
  return total;
}

FoldResult foldNested(const std::string &sequence, int minLoop, int canonicalScore, int wobbleScore) {
  const auto nResult = ribo::checked::narrow<int>(sequence.size());
  if (!nResult) return {0, std::string(sequence.size(), '.')};
  const int n = *nResult;
  if (minLoop < 0 || canonicalScore < 0 || wobbleScore < 0 || !validSequence(sequence)) return {0, std::string(n, '.')};
  const auto cellCount = static_cast<size_t>(n) * static_cast<size_t>(n + 1) / 2;
  std::vector<int> dp(cellCount);
  const auto cell = [n](int i, int j) -> size_t {
    return static_cast<size_t>(i) * static_cast<size_t>(n) - static_cast<size_t>(i) * static_cast<size_t>(i - 1) / 2 +
           static_cast<size_t>(j - i);
  };
  const auto scoreAt = [&](int i, int j) -> int & { return dp[cell(i, j)]; };
  const auto valueAt = [&](int i, int j) -> int { return i >= j ? 0 : scoreAt(i, j); };
  for (int width = 1; width < n; ++width) {
    for (int i = 0, j = width; j < n; ++i, ++j) {
      scoreAt(i, j) = std::max(valueAt(i + 1, j), valueAt(i, j - 1));
      const int score = pairScore(sequence[i], sequence[j], canonicalScore, wobbleScore);
      if (j - i > minLoop && score > 0) scoreAt(i, j) = std::max(scoreAt(i, j), valueAt(i + 1, j - 1) + score);
      for (int split = i + 1; split < j; ++split)
        scoreAt(i, j) = std::max(scoreAt(i, j), valueAt(i, split) + valueAt(split + 1, j));
    }
  }

  std::string structure(n, '.');
  std::function<void(int, int)> traceback = [&](int i, int j) {
    if (i >= j) return;
    if (scoreAt(i, j) == valueAt(i + 1, j)) {
      traceback(i + 1, j);
      return;
    }
    if (scoreAt(i, j) == valueAt(i, j - 1)) {
      traceback(i, j - 1);
      return;
    }
    const int score = pairScore(sequence[i], sequence[j], canonicalScore, wobbleScore);
    if (j - i > minLoop && score > 0 && scoreAt(i, j) == valueAt(i + 1, j - 1) + score) {
      structure[i] = '(';
      structure[j] = ')';
      traceback(i + 1, j - 1);
      return;
    }
    for (int split = i + 1; split < j; ++split) {
      if (scoreAt(i, j) == valueAt(i, split) + valueAt(split + 1, j)) {
        traceback(i, split);
        traceback(split + 1, j);
        return;
      }
    }
  };
  if (n) traceback(0, n - 1);
  return {n ? scoreAt(0, n - 1) : 0, structure};
}

FoldResult foldHType(const std::string &sequence, int minLoop, int stemLength, int canonicalScore, int wobbleScore) {
  return foldHType(sequence, minLoop, stemLength, stemLength, canonicalScore, wobbleScore);
}

FoldResult foldHType(const std::string &sequence, int minLoop, int stem1Length, int stem2Length, int canonicalScore, int wobbleScore) {
  return foldHType(sequence, minLoop, minLoop, minLoop, stem1Length, stem2Length, canonicalScore, wobbleScore);
}

FoldResult foldHType(const std::string &sequence, int gap1MinLoop, int gap2MinLoop, int gap3MinLoop, int stem1Length, int stem2Length, int canonicalScore, int wobbleScore) {
  const auto nResult = ribo::checked::narrow<int>(sequence.size());
  if (!nResult) return {0, std::string(sequence.size(), '.')};
  const int n = *nResult;
  if (gap1MinLoop < 0 || gap2MinLoop < 0 || gap3MinLoop < 0 || stem1Length < 1 || stem2Length < 1 || canonicalScore < 0 || wobbleScore < 0 || !validSequence(sequence)) return {0, std::string(n, '.')};
  int best = 0, ba = -1, bc = -1, bb = -1, bd = -1;
  const auto pair = [&](int i, int j) { return pairScore(sequence[i], sequence[j], canonicalScore, wobbleScore); };
  for (int a = 0; a + stem1Length <= n; ++a) for (int c = a + stem1Length + gap1MinLoop; c + stem2Length <= n; ++c)
    for (int b = c + stem2Length + gap2MinLoop; b + stem1Length <= n; ++b) for (int d = b + stem1Length + gap3MinLoop; d + stem2Length <= n; ++d) {
      int score = 0; bool valid = true;
      for (int k = 0; k < stem1Length; ++k) { const int s1 = pair(a + k, b + stem1Length - 1 - k); if (!s1) { valid = false; break; } score += s1; }
      for (int k = 0; valid && k < stem2Length; ++k) { const int s2 = pair(c + k, d + stem2Length - 1 - k); if (!s2) { valid = false; break; } score += s2; }
      if (valid && score > best) best = score, ba = a, bc = c, bb = b, bd = d;
    }
  std::string structure(n, '.');
  if (ba >= 0) { for (int k = 0; k < stem1Length; ++k) { structure[ba + k] = '('; structure[bb + stem1Length - 1 - k] = ')'; } for (int k = 0; k < stem2Length; ++k) { structure[bc + k] = '['; structure[bd + stem2Length - 1 - k] = ']'; } }
  return {best, structure};
}

}
