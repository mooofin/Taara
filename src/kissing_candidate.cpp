#include "ribo/KissingCandidate.h"
#include "mlir/IR/BuiltinAttributes.h"
#include <algorithm>
#include <array>
using namespace mlir;
namespace ribo {
namespace { static bool compatible(char a, char b) { if (a >= 'a' && a <= 'z') a = static_cast<char>(a - 'a' + 'A'); if (b >= 'a' && b <= 'z') b = static_cast<char>(b - 'a' + 'A'); return (a == 'A' && b == 'U') || (a == 'U' && b == 'A') || (a == 'C' && b == 'G') || (a == 'G' && b == 'C') || (a == 'G' && b == 'U') || (a == 'U' && b == 'G'); } }
LogicalResult KissingCandidateOp::verify() {
  auto rna = getOperand().getDefiningOp(); auto sequence = rna ? rna->getAttrOfType<StringAttr>("sequence") : StringAttr(); if (!rna || rna->getName().getStringRef() != "ribo.rna" || !sequence) return emitError("candidate must consume a ribo.rna result");
  constexpr StringRef e[] = {"stem1_start", "stem1_end", "stem2_start", "stem2_end", "stem3_start", "stem3_end"}; constexpr StringRef len[] = {"stem1_length", "stem2_length", "stem3_length"}; std::array<int64_t, 6> c;
  for (size_t i = 0; i < std::size(e); ++i) { auto v = (*this)->getAttrOfType<IntegerAttr>(e[i]); if (!v || v.getInt() < 0) return emitError() << e[i] << " must be a non-negative integer"; c[i] = v.getInt(); }
  for (int s = 0; s < 3; ++s) { auto a = (*this)->getAttrOfType<IntegerAttr>(e[2 * s]); auto b = (*this)->getAttrOfType<IntegerAttr>(e[2 * s + 1]); auto h = (*this)->getAttrOfType<IntegerAttr>(len[s]); if (!h || h.getInt() < 1) return emitError() << len[s] << " must be a positive integer"; if (b.getInt() <= a.getInt()) return emitError() << "stem" << s + 1 << " end must be greater than start"; if (b.getInt() >= static_cast<int64_t>(sequence.getValue().size())) return emitError() << "stem" << s + 1 << " endpoint exceeds RNA length"; if (h.getInt() > (b.getInt() - a.getInt() + 1) / 2) return emitError() << len[s] << " exceeds its endpoint interval"; for (int k = 0; k < h.getInt(); ++k) if (!compatible(sequence.getValue()[a.getInt() + k], sequence.getValue()[b.getInt() - k])) return emitError() << "stem" << s + 1 << " contains a chemically incompatible base pair"; }
  std::ranges::sort(c); if (std::ranges::adjacent_find(c) != c.end()) return emitError("kissing stems cannot reuse nucleotide endpoints");
  constexpr StringRef l[] = {"loop1_min", "loop2_min", "loop3_min", "loop4_min", "loop5_min"}; for (StringRef k : l) if (auto v = (*this)->getAttrOfType<IntegerAttr>(k); !v || v.getInt() < 0) return emitError() << k << " must be a non-negative integer";
  auto shared = (*this)->getAttrOfType<IntegerAttr>("shared_stem"); if (!shared || shared.getInt() < 1 || shared.getInt() > 3) return emitError("shared_stem must be 1, 2, or 3"); auto once = (*this)->getAttrOfType<BoolAttr>("shared_stem_score_once"); if (!once || !once.getValue()) return emitError("shared_stem_score_once must be true"); auto weight = (*this)->getAttrOfType<IntegerAttr>("weight"); if (!weight || weight.getInt() < 0) return emitError("weight must be a non-negative integer"); if (auto pair = (*this)->getAttrOfType<IntegerAttr>("pair_score"); pair && pair.getInt() < 0) return emitError("pair_score must be non-negative"); if (auto wobble = (*this)->getAttrOfType<IntegerAttr>("wobble_score"); wobble && wobble.getInt() < 0) return emitError("wobble_score must be non-negative"); return success();
}
}
