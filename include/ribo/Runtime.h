#pragma once

#include <string>
#include <span>
#include <array>
#include <cstdint>
#include <utility>

namespace ribo::runtime {

struct FoldResult {
  int score = 0;
  std::string structure;
};

[[nodiscard]] FoldResult foldNested(const std::string &sequence, int minLoop,
                      int canonicalScore, int wobbleScore = 0);
[[nodiscard]] FoldResult foldHType(const std::string &sequence, int minLoop,
                                   int stemLength, int canonicalScore, int wobbleScore = 0);
[[nodiscard]] FoldResult foldHType(const std::string &sequence, int minLoop,
                                   int stem1Length, int stem2Length, int canonicalScore, int wobbleScore);
[[nodiscard]] FoldResult foldHType(const std::string &sequence, int gap1MinLoop, int gap2MinLoop,
                                   int gap3MinLoop, int stem1Length, int stem2Length,
                                   int canonicalScore, int wobbleScore = 0);

[[nodiscard]] std::int64_t scoreStem(std::span<const char> sequence, int start, int end,
                                      int length, int canonicalScore, int wobbleScore = 0);
struct KissingStem { int start = 0, end = 0, length = 0; };
[[nodiscard]] std::int64_t scoreKissingCandidate(std::span<const char> sequence,
                                                 std::array<KissingStem, 3> stems,
                                                 int sharedStem, int canonicalScore,
                                                 int wobbleScore = 0);

}
