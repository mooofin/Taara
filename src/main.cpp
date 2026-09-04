#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/DialectImplementation.h"
#include "mlir/Parser/Parser.h"
#include "ribo/Runtime.h"
#include "ribo/Constraints.h"
#include "ribo/Lowering.h"
#include "ribo/Sequence.h"
#include "ribo/KissingCandidate.h"
#include "ribo/RnaOp.h"
#include "ribo/ConstraintOp.h"
#include "ribo/LoweredFoldOp.h"
#include "ribo/CandidateSelection.h"
#include "ribo/Energy.h"
#include "ribo/Json.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>
using namespace mlir;
namespace ribo {
enum class Topology { Nested, HType, KissingHairpin, BoundedCrossing, GeneralCrossing };
static StringRef name(Topology t) {
  switch (t) {
  case Topology::Nested:
    return "nested";
  case Topology::HType:
    return "h_type_pseudoknot";
  case Topology::KissingHairpin:
    return "kissing_hairpin";
  case Topology::BoundedCrossing:
    return "bounded_crossing";
  case Topology::GeneralCrossing:
    return "general_crossing";
  }
  llvm_unreachable("bad topology");
}
static FailureOr<Topology> parseTopology(StringRef s) {
  for (auto t : {Topology::Nested, Topology::HType, Topology::KissingHairpin, Topology::BoundedCrossing,
                 Topology::GeneralCrossing})
    if (s == name(t)) return t;
  return failure();
}
struct TopologyAttrStorage : AttributeStorage {
  using KeyTy = unsigned;
  unsigned value;
  explicit TopologyAttrStorage(unsigned v) : value(v) {}
  bool operator==(const KeyTy &key) const { return value == key; }
  static TopologyAttrStorage *construct(AttributeStorageAllocator &allocator, const KeyTy &key) {
    return new (allocator.allocate<TopologyAttrStorage>()) TopologyAttrStorage(key);
  }
};
class TopologyAttr : public Attribute::AttrBase<TopologyAttr, Attribute, TopologyAttrStorage> {
public:
  using Base::Base;
  static constexpr StringLiteral name = "topology";
  static TopologyAttr get(MLIRContext *context, Topology topology) { return Base::get(context, static_cast<unsigned>(topology)); }
  Topology getValue() const { return static_cast<Topology>(getImpl()->value); }
};
static FailureOr<Topology> getTopology(Operation *op) {
  if (auto attr = op->getAttrOfType<TopologyAttr>("topology")) return attr.getValue();
  if (auto attr = op->getAttrOfType<StringAttr>("topology")) return parseTopology(attr.getValue());
  return failure();
}
class FoldOp : public Op<FoldOp, OpTrait::OneResult, OpTrait::VariadicOperands> {
public:
  using Op::Op;
  static StringRef getOperationName() { return "ribo.fold"; }
  static ArrayRef<StringRef> getAttributeNames() {
    static StringRef n[] = {"topology", "min_loop", "gap1_min_loop", "gap2_min_loop", "gap3_min_loop", "pair_score", "wobble_score", "stem_length", "stem1_length", "stem2_length", "max_crossing_depth", "state_width", "shape_slope", "shape_intercept", "model", "temperature_k", "sodium_m", "magnesium_m", "parameter_set"};
    return n;
  }
  LogicalResult verify() {
    for (auto attr : (*this)->getAttrs())
      if (auto integer = dyn_cast<IntegerAttr>(attr.getValue()); integer &&
          integer.getInt() > std::numeric_limits<int>::max())
        return emitError() << attr.getName().str() << " exceeds the runtime 32-bit limit";
    auto t = getTopology(*this);
    auto l = (*this)->getAttrOfType<IntegerAttr>("min_loop");
    auto s = (*this)->getAttrOfType<IntegerAttr>("pair_score");
    if (getNumOperands() < 1 || getNumOperands() > 2)
      return emitError("fold expects an RNA operand and optional constraint");
    auto r = llvm::dyn_cast<RankedTensorType>(getOperand(0).getType());
    if (!r || r.getRank() != 1 || !r.getElementType().isInteger(8))
      return emitError("RNA operand must be a ranked tensor<*xi8>");
    if (failed(t)) return emitError("invalid topology");
    if (getNumOperands() == 2) {
      auto constraint = getOperand(1).getDefiningOp();
      if (!constraint || constraint->getName().getStringRef() != "ribo.constraint")
        return emitError("second fold operand must be a ribo.constraint result");
      if (constraint->getNumOperands() != 1 || constraint->getOperand(0) != getOperand(0))
        return emitError("constraint must be attached to the same RNA operand");
    }
    for (StringRef key : {StringRef("shape_slope"), StringRef("shape_intercept")})
      if (auto calibration = (*this)->getAttrOfType<FloatAttr>(key))
        if (!std::isfinite(calibration.getValueAsDouble())) return emitError("SHAPE calibration must be finite");
    if (auto slope = (*this)->getAttrOfType<FloatAttr>("shape_slope"))
      if (slope.getValueAsDouble() <= 0.0) return emitError("SHAPE slope must be positive");
    auto model = (*this)->getAttrOfType<StringAttr>("model");
      if (model && model.getValue() != "reference_nussinov" && model.getValue() != "nearest_neighbor")
      return emitError("model must be reference_nussinov or nearest_neighbor");
    const bool hasEnergy = (*this)->getAttr("temperature_k") || (*this)->getAttr("sodium_m") || (*this)->getAttr("magnesium_m") || (*this)->getAttr("parameter_set");
    if (hasEnergy) {
      const auto temperature = (*this)->getAttrOfType<FloatAttr>("temperature_k");
      const auto sodium = (*this)->getAttrOfType<FloatAttr>("sodium_m");
      const auto magnesium = (*this)->getAttrOfType<FloatAttr>("magnesium_m");
      const auto parameters = (*this)->getAttrOfType<StringAttr>("parameter_set");
      if (!temperature || !sodium || !magnesium || !parameters) return emitError("thermodynamic metadata requires temperature_k, sodium_m, magnesium_m, and parameter_set");
      std::string energyError;
      if (!ribo::energy::valid({temperature.getValueAsDouble(), sodium.getValueAsDouble(), magnesium.getValueAsDouble(), parameters.getValue().str()}, &energyError)) return emitError() << energyError;
    }
    if (!l || l.getInt() < 0) return emitError("min_loop must be non-negative");
    if (!s || s.getInt() < 0) return emitError("pair_score must be a non-negative integer");
    for (StringRef key : {StringRef("gap1_min_loop"), StringRef("gap2_min_loop"), StringRef("gap3_min_loop")})
      if (auto gap = (*this)->getAttrOfType<IntegerAttr>(key); gap && gap.getInt() < 0) return emitError("H-type loop minima must be non-negative");
    auto wb = (*this)->getAttrOfType<IntegerAttr>("wobble_score");
    if (wb && wb.getInt() < 0) return emitError("wobble_score must be non-negative");
    if (succeeded(t) && *t == Topology::HType) {
      auto stem = (*this)->getAttrOfType<IntegerAttr>("stem_length");
      auto stem1 = (*this)->getAttrOfType<IntegerAttr>("stem1_length");
      auto stem2 = (*this)->getAttrOfType<IntegerAttr>("stem2_length");
      if (!stem && (!stem1 || !stem2)) return emitError("h_type_pseudoknot requires stem_length or both stem1_length/stem2_length");
      if ((stem && stem.getInt() < 1) || (stem1 && stem1.getInt() < 1) || (stem2 && stem2.getInt() < 1)) return emitError("H-type stem lengths must be positive");
      if ((stem1 && !stem2) || (!stem1 && stem2)) return emitError("H-type requires both stem1_length and stem2_length");
    }
    if (succeeded(t) && *t != Topology::HType)
      for (StringRef key : {StringRef("stem_length"), StringRef("stem1_length"), StringRef("stem2_length"), StringRef("gap1_min_loop"), StringRef("gap2_min_loop"), StringRef("gap3_min_loop")})
        if ((*this)->getAttr(key)) return emitError() << "attribute '" << key << "' is only valid for h_type_pseudoknot";
    if (succeeded(t) && *t == Topology::BoundedCrossing) {
      auto depth = (*this)->getAttrOfType<IntegerAttr>("max_crossing_depth");
      if (!depth || depth.getInt() < 1) return emitError("bounded_crossing requires max_crossing_depth >= 1");
      auto width = (*this)->getAttrOfType<IntegerAttr>("state_width");
      if (!width || width.getInt() < 1) return emitError("bounded_crossing requires state_width >= 1");
    }
    return success();
  }
};
static bool chemicallyCompatible(char a, char b) {
  if (a >= 'a' && a <= 'z') a = static_cast<char>(a - 'a' + 'A');
  if (b >= 'a' && b <= 'z') b = static_cast<char>(b - 'a' + 'A');
  return (a == 'A' && b == 'U') || (a == 'U' && b == 'A') || (a == 'C' && b == 'G') || (a == 'G' && b == 'C') ||
         (a == 'G' && b == 'U') || (a == 'U' && b == 'G');
}
static bool structurePairsCompatible(StringRef structure, StringRef sequence) {
  std::vector<std::vector<int>> opens(3);
  for (int i = 0; i < (int)structure.size(); ++i) {
    char c = structure[i];
    if (c == '(')
      opens[0].push_back(i);
    else if (c == '[')
      opens[1].push_back(i);
    else if (c == '{')
      opens[2].push_back(i);
    else if (c == ')' || c == ']' || c == '}') {
      int k = c == ')' ? 0 : c == ']' ? 1 : 2;
      int j = opens[k].back();
      opens[k].pop_back();
      if (!chemicallyCompatible(sequence[j], sequence[i])) return false;
    }
  }
  return true;
}
class StructureOp : public Op<StructureOp, OpTrait::OneOperand, OpTrait::ZeroResults> {
public:
  using Op::Op;
  static StringRef getOperationName() { return "ribo.structure"; }
  static ArrayRef<StringRef> getAttributeNames() {
    static StringRef n[] = {"dot_bracket", "topology"};
    return n;
  }
  LogicalResult verify() {
    auto s = (*this)->getAttrOfType<StringAttr>("dot_bracket");
    if (auto rna = getOperand().getDefiningOp<RnaOp>()) {
      auto seq = rna->getAttrOfType<StringAttr>("sequence");
      if (seq && seq.getValue().size() != s.getValue().size())
        return emitError("dot_bracket length must match RNA sequence length");
    }
    if (!s || s.getValue().empty()) return emitError("dot_bracket structure must be non-empty");
    std::vector<std::vector<int>> opens(3);
    std::vector<std::pair<int, int>> pairs;
    unsigned bracketFamilies = 0;
    for (int i = 0; i < (int)s.getValue().size(); ++i) {
      char c = s.getValue()[i];
      if (c == '(')
        opens[0].push_back(i), bracketFamilies |= 1u;
      else if (c == '[')
        opens[1].push_back(i), bracketFamilies |= 2u;
      else if (c == '{')
        opens[2].push_back(i), bracketFamilies |= 4u;
      else if (c == ')' || c == ']' || c == '}') {
        int k = c == ')' ? 0 : c == ']' ? 1 : 2;
        if (opens[k].empty()) return emitError("dot_bracket has an unmatched closing delimiter");
        int j = opens[k].back();
        opens[k].pop_back();
        pairs.push_back({j, i});
      } else if (c != '.')
        return emitError("dot_bracket may contain only ., (), [], and {}");
    }
    if (!opens[0].empty() || !opens[1].empty() || !opens[2].empty()) return emitError("dot_bracket has an unmatched opening delimiter");
    bool crossing = false;
    for (auto a : pairs)
      for (auto b : pairs)
        if (a.first < b.first && b.first < a.second && a.second < b.second) crossing = true;
    auto t = getTopology(*this);
    if (failed(t) && (*this)->getAttr("topology")) return emitError("invalid topology");
    if (auto rna = getOperand().getDefiningOp<RnaOp>()) {
      auto seq = rna->getAttrOfType<StringAttr>("sequence");
      if (seq && !structurePairsCompatible(s.getValue(), seq.getValue()))
        return emitError("dot_bracket contains a chemically incompatible base pair");
    }
    if (succeeded(t) && *t == Topology::Nested && crossing)
      return emitError("nested topology cannot contain crossing base pairs");
    if (succeeded(t) && *t == Topology::KissingHairpin && (!crossing || bracketFamilies != 7u))
      return emitError("kissing_hairpin requires a crossing structure with all three stem alphabets (), [], and {}");
    return success();
  }
};
class RiboDialect : public Dialect {
public:
  static StringRef getDialectNamespace() { return "ribo"; }
  explicit RiboDialect(MLIRContext *c) : Dialect("ribo", c, TypeID::get<RiboDialect>()) {
    addAttributes<TopologyAttr>();
    addOperations<FoldOp, ribo::LoweredFoldOp, ribo::RnaOp, StructureOp, ribo::ConstraintOp, ribo::KissingCandidateOp>();
  }
  Attribute parseAttribute(DialectAsmParser &parser, Type) const override {
    StringRef keyword;
    if (failed(parser.parseKeyword(&keyword))) return {};
    auto topology = parseTopology(keyword);
    if (failed(topology)) { parser.emitError(parser.getNameLoc()) << "unknown ribo topology '" << keyword << "'"; return {}; }
    return TopologyAttr::get(getContext(), *topology);
  }
  void printAttribute(Attribute attribute, DialectAsmPrinter &printer) const override {
    printer << ribo::name(mlir::dyn_cast<TopologyAttr>(attribute).getValue());
  }
};
}
int main(int argc, char **argv) {
  DialectRegistry r;
  r.insert<ribo::RiboDialect>();
  MLIRContext c(r);
  c.loadDialect<ribo::RiboDialect>();
  c.allowUnregisteredDialects(true);
  if (argc >= 2 && std::string(argv[1]) == "--version") {
    std::cout << "Tara " << BIO_LLVM_VERSION << " (MLIR " << BIO_LLVM_MLIR_VERSION << "; topology-aware RNA prototype)\n";
    return 0;
  }
  if (argc >= 2 && std::string(argv[1]) == "--help") {
    std::cerr << "usage: bio-llvm <file.mlir> [SEQUENCE|--plan|--plan-json]\n";
    std::cerr << "       bio-llvm <file.mlir> --lower\n";
    std::cerr << "       bio-llvm <file.mlir> --select\n";
    std::cerr << "       bio-llvm <file.mlir> --select-sequence\n";
    std::cerr << "       bio-llvm <file.mlir> --select-sequence-json\n";
    std::cerr << "       bio-llvm <file.mlir> --select-json\n";
    std::cerr << "       bio-llvm <file.mlir> --classify\n";
    std::cerr << "       bio-llvm <file.mlir> --classify-strict\n";
    std::cerr << "       bio-llvm <file.mlir> --profile\n";
    std::cerr << "       bio-llvm --version\n";
    return 0;
  }
  if (argc < 2) {
    std::cerr << "usage: bio-llvm <file.mlir> [SEQUENCE|--plan|--plan-json]\n";
    return 2;
  }
  auto m = parseSourceFile<ModuleOp>(argv[1], &c);
  if (!m) return 1;
  bool ok = true;
  if (argc >= 3 && (std::string(argv[2]) == "--select" || std::string(argv[2]) == "--select-json" || std::string(argv[2]) == "--select-sequence" || std::string(argv[2]) == "--select-sequence-json")) {
    const bool sequenceSelection = std::string(argv[2]) == "--select-sequence" || std::string(argv[2]) == "--select-sequence-json";
    std::vector<ribo::selection::Candidate> candidates;
    Operation *selectionRna = nullptr;
    bool mixedSequences = false;
    m->walk([&](ribo::KissingCandidateOp candidate) {
      auto *rna = candidate.getOperand().getDefiningOp();
      if (!selectionRna) selectionRna = rna; else if (selectionRna != rna) mixedSequences = true;
      ribo::selection::Candidate c; c.weight = candidate->getAttrOfType<IntegerAttr>("weight").getInt();
      if (sequenceSelection) {
        const auto seq = rna->getAttrOfType<StringAttr>("sequence").getValue().str();
        const std::array<ribo::runtime::KissingStem, 3> stems{{
          {static_cast<int>(candidate->getAttrOfType<IntegerAttr>("stem1_start").getInt()), static_cast<int>(candidate->getAttrOfType<IntegerAttr>("stem1_end").getInt()), static_cast<int>(candidate->getAttrOfType<IntegerAttr>("stem1_length").getInt())},
          {static_cast<int>(candidate->getAttrOfType<IntegerAttr>("stem2_start").getInt()), static_cast<int>(candidate->getAttrOfType<IntegerAttr>("stem2_end").getInt()), static_cast<int>(candidate->getAttrOfType<IntegerAttr>("stem2_length").getInt())},
          {static_cast<int>(candidate->getAttrOfType<IntegerAttr>("stem3_start").getInt()), static_cast<int>(candidate->getAttrOfType<IntegerAttr>("stem3_end").getInt()), static_cast<int>(candidate->getAttrOfType<IntegerAttr>("stem3_length").getInt())}}};
        const auto wobble = candidate->getAttrOfType<IntegerAttr>("wobble_score");
        const auto pair = candidate->getAttrOfType<IntegerAttr>("pair_score");
        c.weight = ribo::runtime::scoreKissingCandidate(seq, stems, static_cast<int>(candidate->getAttrOfType<IntegerAttr>("shared_stem").getInt()), pair ? static_cast<int>(pair.getInt()) : 1, wobble ? static_cast<int>(wobble.getInt()) : 0);
      }
      for (int s = 1; s <= 3; ++s) {
        const auto start = candidate->getAttrOfType<IntegerAttr>("stem" + std::to_string(s) + "_start").getInt();
        const auto end = candidate->getAttrOfType<IntegerAttr>("stem" + std::to_string(s) + "_end").getInt();
        const auto length = candidate->getAttrOfType<IntegerAttr>("stem" + std::to_string(s) + "_length").getInt();
        for (int k = 0; k < length; ++k) c.occupied.push_back(static_cast<int>(start + k)), c.occupied.push_back(static_cast<int>(end - k));
      }
      candidates.push_back(std::move(c));
    });
    if (mixedSequences) { std::cerr << "selection requires all candidates to consume the same RNA operand\n"; return 1; }
    const auto result = ribo::selection::selectMaximumWeight(candidates);
    if (std::string(argv[2]) == "--select-json" || std::string(argv[2]) == "--select-sequence-json") {
      std::cout << "{\"schema\":\"ribo.selection.v1\",\"selected_weight\":" << result.weight << ",\"indices\":[";
      for (size_t i = 0; i < result.selected.size(); ++i) std::cout << (i ? "," : "") << result.selected[i];
      std::cout << "]}\n";
      return 0;
    }
    std::cout << "selected_weight=" << result.weight << " candidates=" << result.selected.size() << " indices=";
    for (size_t i = 0; i < result.selected.size(); ++i) std::cout << (i ? "," : "") << result.selected[i];
    std::cout << "\n";
    return 0;
  }
  if (argc >= 3 && (std::string(argv[2]) == "--classify" || std::string(argv[2]) == "--classify-strict")) {
    const bool strict = std::string(argv[2]) == "--classify-strict";
    m->walk([&](ribo::StructureOp structure) {
      const auto dot = structure->getAttrOfType<StringAttr>("dot_bracket").getValue();
      bool crossing = false;
      unsigned bracketFamilies = 0;
      std::vector<std::pair<int, int>> pairs;
      std::array<std::vector<int>, 3> opens;
      for (int i = 0; i < static_cast<int>(dot.size()); ++i) {
        if (dot[i] == '(' || dot[i] == '[' || dot[i] == '{') {
          const int kind = dot[i] == '[' ? 1 : dot[i] == '{' ? 2 : 0;
          bracketFamilies |= 1u << kind;
          opens[kind].push_back(i);
        }
        if (dot[i] == ')' || dot[i] == ']' || dot[i] == '}') {
          const int kind = dot[i] == ')' ? 0 : dot[i] == ']' ? 1 : 2;
          pairs.emplace_back(opens[kind].back(), i);
          opens[kind].pop_back();
        }
      }
      for (const auto &[a, b] : pairs)
        for (const auto &[c, d] : pairs)
          if (a < c && c < b && b < d) crossing = true;
      int maxOverlap = 0;
      for (int i = 0; i < static_cast<int>(dot.size()); ++i) {
        int overlap = 0;
        for (const auto &[a, b] : pairs)
          if (a <= i && i < b) ++overlap;
        maxOverlap = std::max(maxOverlap, overlap);
      }
      const auto observed = crossing ? StringRef("crossing") : StringRef("nested");
      const auto family = !crossing ? StringRef("nested") : bracketFamilies == 3u ? StringRef("h_type_pseudoknot")
                                                                                   : bracketFamilies == 7u ? StringRef("kissing_hairpin_candidate")
                                                                                                           : StringRef("crossing");
      const auto declared = ribo::getTopology(structure);
      std::cout << "observed_topology=" << observed.str() << "\n" << "base_pairs=" << pairs.size() << "\n"
                << "max_pair_overlap=" << maxOverlap << "\n"
                << "observed_family=" << family.str() << "\n"
                << "pair_indices=";
      for (size_t i = 0; i < pairs.size(); ++i)
        std::cout << (i ? "," : "") << pairs[i].first + 1 << "-" << pairs[i].second + 1;
      std::cout << "\n";
      if (succeeded(declared)) {
        const bool declaresCrossing = *declared != ribo::Topology::Nested;
        const bool consistent = declaresCrossing == crossing;
        std::cout << "declared_topology=" << ribo::name(*declared).str() << "\n"
                  << "topology_consistent=" << (consistent ? "yes" : "no") << "\n";
        if (strict && !consistent) ok = false;
      }
    });
    return ok ? 0 : 1;
  }
  if (argc >= 3 && (std::string(argv[2]) == "--plan" || std::string(argv[2]) == "--plan-json"))
    m->walk([&](ribo::KissingCandidateOp candidate) {
      const auto shared = candidate->getAttrOfType<IntegerAttr>("shared_stem").getInt();
      const auto once = candidate->getAttrOfType<BoolAttr>("shared_stem_score_once").getValue();
      const auto h1 = candidate->getAttrOfType<IntegerAttr>("stem1_length").getInt();
      const auto h2 = candidate->getAttrOfType<IntegerAttr>("stem2_length").getInt();
      const auto h3 = candidate->getAttrOfType<IntegerAttr>("stem3_length").getInt();
      const auto a1 = candidate->getAttrOfType<IntegerAttr>("stem1_start").getInt();
      const auto b1 = candidate->getAttrOfType<IntegerAttr>("stem1_end").getInt();
      const auto a2 = candidate->getAttrOfType<IntegerAttr>("stem2_start").getInt();
      const auto b2 = candidate->getAttrOfType<IntegerAttr>("stem2_end").getInt();
      const auto a3 = candidate->getAttrOfType<IntegerAttr>("stem3_start").getInt();
      const auto b3 = candidate->getAttrOfType<IntegerAttr>("stem3_end").getInt();
      const auto l1 = candidate->getAttrOfType<IntegerAttr>("loop1_min").getInt();
      const auto l2 = candidate->getAttrOfType<IntegerAttr>("loop2_min").getInt();
      const auto l3 = candidate->getAttrOfType<IntegerAttr>("loop3_min").getInt();
      const auto l4 = candidate->getAttrOfType<IntegerAttr>("loop4_min").getInt();
      const auto l5 = candidate->getAttrOfType<IntegerAttr>("loop5_min").getInt();
      const auto weight = candidate->getAttrOfType<IntegerAttr>("weight").getInt();
      const auto wobbleScore = candidate->getAttrOfType<IntegerAttr>("wobble_score");
      const auto pairScore = candidate->getAttrOfType<IntegerAttr>("pair_score");
      const auto sequence = candidate.getOperand().getDefiningOp<ribo::RnaOp>()->getAttrOfType<StringAttr>("sequence").getValue().str();
      const auto sequenceScore = ribo::runtime::scoreKissingCandidate(sequence, std::array<ribo::runtime::KissingStem, 3>{{{static_cast<int>(a1), static_cast<int>(b1), static_cast<int>(h1)}, {static_cast<int>(a2), static_cast<int>(b2), static_cast<int>(h2)}, {static_cast<int>(a3), static_cast<int>(b3), static_cast<int>(h3)}}}, static_cast<int>(shared), pairScore ? static_cast<int>(pairScore.getInt()) : 1, wobbleScore ? static_cast<int>(wobbleScore.getInt()) : 0);
      if (std::string(argv[2]) == "--plan-json")
        std::cout << "{\"schema\":\"ribo.plan.v1\",\"kind\":\"kissing_hairpin_candidate\",\"stems\":[[" << a1 << "," << b1 << "],[" << a2 << "," << b2 << "],[" << a3 << "," << b3 << " ]],\"loops\":[" << l1 << "," << l2 << "," << l3 << "," << l4 << "," << l5 << "],\"stem_lengths\":["
                  << h1 << "," << h2 << "," << h3 << "],\"weight\":" << weight << ",\"sequence_score\":" << sequenceScore << ",\"pair_score\":" << (pairScore ? pairScore.getInt() : 1) << ",\"wobble_score\":" << (wobbleScore ? wobbleScore.getInt() : 0) << ",\"shared_stem\":" << shared
                  << ",\"shared_stem_score_once\":" << (once ? "true" : "false") << "}\n";
      else
        std::cout << "candidate=kissing_hairpin stems=3 lengths=" << h1 << "," << h2 << "," << h3 << " endpoints=" << a1 << "-" << b1 << "," << a2 << "-" << b2 << "," << a3 << "-" << b3 << " loops=" << l1 << "," << l2 << "," << l3 << "," << l4 << "," << l5 << " weight=" << weight << " shared_stem=" << shared
                  << " shared_stem_score_once=" << (once ? "true" : "false") << " sequence_score=" << sequenceScore << " pair_score=" << (pairScore ? pairScore.getInt() : 1) << " wobble_score=" << (wobbleScore ? wobbleScore.getInt() : 0) << "\n";
    });
  m->walk([&](ribo::FoldOp op) {
    auto t = ribo::getTopology(op);
    if (failed(t)) return;
    if (argc >= 3 && std::string(argv[2]) == "--profile") {
      std::string sequence;
      if (auto rna = op.getOperand(0).getDefiningOp<ribo::RnaOp>())
        sequence = rna->getAttrOfType<StringAttr>("sequence").getValue().str();
      sequence = ribo::sequence::normalize(sequence);
      int a = 0, c = 0, g = 0, u = 0;
      for (char base : sequence) {
        if (base == 'A') ++a;
        if (base == 'C') ++c;
        if (base == 'G') ++g;
        if (base == 'U') ++u;
      }
      const int gc = g + c, au = a + u, gu = g + u;
      std::cout << "length=" << sequence.size() << "\n" << "a_count=" << a << "\n" << "c_count=" << c << "\n"
                << "g_count=" << g << "\n" << "u_count=" << u << "\n" << "gc_count=" << gc << "\n"
                << "au_count=" << au << "\n" << "gu_count=" << gu << "\n"
                << "gc_fraction=" << (sequence.empty() ? 0.0 : static_cast<double>(gc) / sequence.size()) << "\n";
      return;
    }
    const bool planJson = argc >= 3 && std::string(argv[2]) == "--plan-json";
    bool plan = argc >= 3 && (std::string(argv[2]) == "--plan" || planJson);
    bool lower = argc >= 3 && std::string(argv[2]) == "--lower";
    if (plan) {
      auto p = ribo::lowering::select(ribo::name(*t).str(),
                                      op->getAttrOfType<IntegerAttr>("max_crossing_depth")
                                          ? std::optional<long long>(op->getAttrOfType<IntegerAttr>("max_crossing_depth").getInt())
                                          : std::nullopt,
                                      op->getAttrOfType<IntegerAttr>("state_width")
                                          ? std::optional<long long>(op->getAttrOfType<IntegerAttr>("state_width").getInt())
                                          : std::nullopt);
      if (planJson) {
        std::cout << "{\"schema\":\"ribo.plan.v1\",\"kind\":\"fold_plan\",\"topology\":\"" << ribo::name(*t).str()
                  << "\",\"recurrence\":\"" << p.recurrence << "\",\"decomposition\":\""
                  << p.decomposition << "\",\"state_rank\":\"" << p.stateRank << "\",\"time\":\""
                  << p.time << "\",\"space\":\"" << p.space << "\",\"strategy\":\"" << p.strategy
                  << "\",\"optimality\":\"" << p.optimality << "\",\"supported\":"
                  << (p.supported ? "true" : "false") << ",\"sparsifiable\":"
                  << (p.sparsifiable ? "true" : "false");
        if (p.crossingDepth) std::cout << ",\"max_crossing_depth\":" << *p.crossingDepth;
        if (p.stateWidth) std::cout << ",\"state_width\":" << *p.stateWidth;
        if (auto value = op->getAttrOfType<FloatAttr>("temperature_k")) std::cout << ",\"temperature_k\":" << value.getValueAsDouble();
        if (auto value = op->getAttrOfType<FloatAttr>("sodium_m")) std::cout << ",\"sodium_m\":" << value.getValueAsDouble();
        if (auto value = op->getAttrOfType<FloatAttr>("magnesium_m")) std::cout << ",\"magnesium_m\":" << value.getValueAsDouble();
        if (auto value = op->getAttrOfType<StringAttr>("parameter_set")) std::cout << ",\"parameter_set\":\"" << ribo::json::escape(value.getValue()) << "\"";
        std::cout << "}\n";
        return;
      }
      std::cout << "topology=" << ribo::name(*t).str() << "\n"
                << "recurrence=" << p.recurrence << "\ndecomposition=" << p.decomposition << "\nstate_rank=" << p.stateRank << "\ntime=" << p.time
                << "\nspace=" << p.space << "\nstrategy=" << p.strategy << "\noptimality=" << p.optimality
                << "\nsparsifiable=" << (p.sparsifiable ? "yes" : "no") << "\n";
      if (p.crossingDepth) std::cout << "max_crossing_depth=" << *p.crossingDepth << "\n";
      if (p.stateWidth) std::cout << "state_width=" << *p.stateWidth << "\n";
      if (auto value = op->getAttrOfType<FloatAttr>("temperature_k")) std::cout << "temperature_k=" << value.getValueAsDouble() << "\n";
      if (auto value = op->getAttrOfType<FloatAttr>("sodium_m")) std::cout << "sodium_m=" << value.getValueAsDouble() << "\n";
      if (auto value = op->getAttrOfType<FloatAttr>("magnesium_m")) std::cout << "magnesium_m=" << value.getValueAsDouble() << "\n";
      if (auto value = op->getAttrOfType<StringAttr>("parameter_set")) std::cout << "parameter_set=" << value.getValue().str() << "\n";
      if (op.getNumOperands() == 2) std::cout << "constraint=shape-soft\n";
      return;
    }
    if (lower) {
      if (auto model = op->getAttrOfType<StringAttr>("model"); model && model.getValue() == "nearest_neighbor") {
        op.emitError() << "model nearest_neighbor is reserved but not implemented; lowering requires an energy backend";
        ok = false;
        return;
      }
      if (*t != ribo::Topology::Nested && *t != ribo::Topology::HType) {
        op.emitError() << "topology '" << ribo::name(*t).str() << "' has no legal lowering; "
                       << (*t == ribo::Topology::GeneralCrossing ? "heuristic/search" : "higher-dimensional")
                       << " recurrence pass is required";
        ok = false;
        return;
      }
      MLIRContext *context = op.getContext();
      OperationState state(op.getLoc(), ribo::LoweredFoldOp::getOperationName());
      state.addAttribute("topology", op->getAttr("topology"));
      const bool hType = *t == ribo::Topology::HType;
      state.addAttribute("strategy", StringAttr::get(context, hType ? "h_type_bounded_dp" : "nested_triangular_dp"));
      state.addAttribute("runtime", StringAttr::get(
          context, hType ? (op.getNumOperands() == 2 ? "ribo-runtime.foldHTypeWithShape" : "ribo-runtime.foldHType") : (op.getNumOperands() == 2 ? "ribo-runtime.foldNestedWithShape" : "ribo-runtime.foldNested")));
      state.addAttribute("state_rank", IntegerAttr::get(IntegerType::get(context, 64), hType ? 4 : 2));
      state.addAttribute("state_variables", StringAttr::get(context, hType ? "a,c,b,d" : "i,j"));
      state.addAttribute("dependencies", StringAttr::get(context, hType ? "stem1(a,b),stem2(c,d),gap1,gap2,gap3" : "[i+1,j],[i,j-1],[i+1,j-1],[i,k]+[k+1,j]"));
      state.addAttribute("storage", StringAttr::get(context, hType ? "bounded_crossing(a,c,b,d)" : "upper_triangular(i,j)"));
      if (hType && op->getAttr("stem_length")) state.addAttribute("stem_length", op->getAttr("stem_length"));
      if (hType && op->getAttr("stem1_length")) { state.addAttribute("stem1_length", op->getAttr("stem1_length")); state.addAttribute("stem2_length", op->getAttr("stem2_length")); }
      if (hType) for (StringRef key : {StringRef("gap1_min_loop"), StringRef("gap2_min_loop"), StringRef("gap3_min_loop")})
        state.addAttribute(key, op->getAttr(key) ? op->getAttr(key) : op->getAttr("min_loop"));
      state.addAttribute("time", StringAttr::get(context, hType ? "O(n^4)" : "O(n^3)"));
      state.addAttribute("space", StringAttr::get(context, hType ? "O(n^4)" : "O(n^2)"));
      state.addAttribute("optimality", StringAttr::get(context, hType ? "restricted-exact" : "exact"));
      if (op.getNumOperands() == 2)
        state.addAttribute("constraint", StringAttr::get(context, "shape-soft"));
      if (op.getNumOperands() == 2) {
        state.addAttribute("shape_slope", op->getAttrOfType<FloatAttr>("shape_slope")
                                             ? op->getAttr("shape_slope")
                                             : FloatAttr::get(Float64Type::get(context), 1.8));
        state.addAttribute("shape_intercept", op->getAttrOfType<FloatAttr>("shape_intercept")
                                                  ? op->getAttr("shape_intercept")
                                                  : FloatAttr::get(Float64Type::get(context), -0.6));
      }
      for (StringRef key : {StringRef("temperature_k"), StringRef("sodium_m"), StringRef("magnesium_m"), StringRef("parameter_set")})
        if (auto value = op->getAttr(key)) state.addAttribute(key, value);
      auto *lowered = Operation::create(state);
      lowered->print(llvm::outs());
      llvm::outs() << "\n";
      lowered->destroy();
      return;
    }
    if (auto model = op->getAttrOfType<StringAttr>("model"))
      if (model.getValue() == "nearest_neighbor") {
        op.emitError() << "model nearest_neighbor is reserved but not implemented; "
                          "reference_nussinov is the executable model";
        ok = false;
        return;
      }
    if (*t != ribo::Topology::Nested && *t != ribo::Topology::HType) {
      op.emitError() << "topology '" << ribo::name(*t).str() << "' requires "
                     << (*t == ribo::Topology::GeneralCrossing ? "heuristic/search" : "higher-dimensional")
                     << " lowering; not implemented";
      ok = false;
      return;
    }
    std::string sequence = argc >= 3 ? argv[2] : "";
    if (!sequence.empty() && sequence[0] == "@"[0]) {
      std::ifstream input(sequence.substr(1));
      if (!input) {
        op.emitError() << "cannot open sequence file " << sequence.substr(1);
        ok = false;
        return;
      }
      sequence.assign((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    }
    if (sequence.empty())
      if (auto rna = op.getOperand(0).getDefiningOp<ribo::RnaOp>())
        sequence = rna->getAttrOfType<StringAttr>("sequence").getValue().str();
    if (sequence.empty()) {
      op.emitError() << "no RNA sequence available; add sequence to ribo.rna "
                        "or pass SEQUENCE";
      ok = false;
      return;
    }
    sequence = ribo::sequence::normalize(sequence);
    ribo::runtime::FoldResult x;
    const int minLoop = op->getAttrOfType<IntegerAttr>("min_loop").getInt();
    const int pairScore = op->getAttrOfType<IntegerAttr>("pair_score").getInt();
    if (*t == ribo::Topology::HType) {
      const auto legacyStem = op->getAttrOfType<IntegerAttr>("stem_length");
      const auto stem1 = op->getAttrOfType<IntegerAttr>("stem1_length");
      const auto stem2 = op->getAttrOfType<IntegerAttr>("stem2_length");
      const auto g1 = op->getAttrOfType<IntegerAttr>("gap1_min_loop"), g2 = op->getAttrOfType<IntegerAttr>("gap2_min_loop"), g3 = op->getAttrOfType<IntegerAttr>("gap3_min_loop");
      const int loop1 = g1 ? static_cast<int>(g1.getInt()) : minLoop, loop2 = g2 ? static_cast<int>(g2.getInt()) : minLoop, loop3 = g3 ? static_cast<int>(g3.getInt()) : minLoop;
      if (op.getNumOperands() == 2) {
        auto values = op.getOperand(1).getDefiningOp()->getAttrOfType<DenseFPElementsAttr>("reactivity"); std::vector<double> r;
        for (const auto &v : values.getValues<APFloat>()) r.push_back(v.convertToDouble());
        ribo::runtime::ShapeCalibration cal; if (auto v = op->getAttrOfType<FloatAttr>("shape_slope")) cal.slope = v.getValueAsDouble(); if (auto v = op->getAttrOfType<FloatAttr>("shape_intercept")) cal.intercept = v.getValueAsDouble();
        x = ribo::runtime::foldHTypeWithShape(sequence, loop1, loop2, loop3, static_cast<int>(stem1 ? stem1.getInt() : legacyStem.getInt()), static_cast<int>(stem2 ? stem2.getInt() : legacyStem.getInt()), pairScore, r, cal, op->getAttrOfType<IntegerAttr>("wobble_score") ? static_cast<int>(op->getAttrOfType<IntegerAttr>("wobble_score").getInt()) : 0);
      } else x = stem1 ? ribo::runtime::foldHType(sequence, loop1, loop2, loop3, static_cast<int>(stem1.getInt()), static_cast<int>(stem2.getInt()), pairScore,
                                            op->getAttrOfType<IntegerAttr>("wobble_score") ? op->getAttrOfType<IntegerAttr>("wobble_score").getInt() : 0)
                 : ribo::runtime::foldHType(sequence, minLoop, static_cast<int>(legacyStem.getInt()), pairScore,
                                            op->getAttrOfType<IntegerAttr>("wobble_score") ? op->getAttrOfType<IntegerAttr>("wobble_score").getInt() : 0);
    } else if (op.getNumOperands() == 2) {
      auto constraint = op.getOperand(1).getDefiningOp();
      auto values = constraint->getAttrOfType<DenseFPElementsAttr>("reactivity");
      std::vector<double> reactivity;
      for (const auto &value : values.getValues<APFloat>()) reactivity.push_back(value.convertToDouble());
      ribo::runtime::ShapeCalibration calibration;
      if (auto slope = op->getAttrOfType<FloatAttr>("shape_slope")) calibration.slope = slope.getValueAsDouble();
      if (auto intercept = op->getAttrOfType<FloatAttr>("shape_intercept")) calibration.intercept = intercept.getValueAsDouble();
      x = ribo::runtime::foldNestedWithShape(sequence, minLoop, pairScore, reactivity, calibration);
    } else {
      x = ribo::runtime::foldNested(sequence, minLoop, pairScore,
                                     op->getAttrOfType<IntegerAttr>("wobble_score") ? op->getAttrOfType<IntegerAttr>("wobble_score").getInt() : 0);
    }
    const auto pairCount = static_cast<int>(std::count(x.structure.begin(), x.structure.end(), '(') +
                                             std::count(x.structure.begin(), x.structure.end(), '[') +
                                             std::count(x.structure.begin(), x.structure.end(), '{'));
    const auto unpaired = static_cast<int>(std::count(x.structure.begin(), x.structure.end(), '.'));
    const auto unpairedFraction = x.structure.empty() ? 0.0 : static_cast<double>(unpaired) / x.structure.size();
    std::vector<int> open, square, curly;
    std::vector<std::string> pairIndices;
    for (int i = 0; i < static_cast<int>(x.structure.size()); ++i) {
      if (x.structure[i] == '(') open.push_back(i);
      if (x.structure[i] == ')' && !open.empty()) {
        pairIndices.emplace_back(std::to_string(open.back() + 1) + "-" + std::to_string(i + 1));
        open.pop_back();
      }
      if (x.structure[i] == '[') square.push_back(i);
      if (x.structure[i] == ']' && !square.empty()) {
        pairIndices.emplace_back(std::to_string(square.back() + 1) + "-" + std::to_string(i + 1));
        square.pop_back();
      }
      if (x.structure[i] == '{') curly.push_back(i);
      if (x.structure[i] == '}' && !curly.empty()) {
        pairIndices.emplace_back(std::to_string(curly.back() + 1) + "-" + std::to_string(i + 1));
        curly.pop_back();
      }
    }
    std::cout << "score=" << x.score << " structure=" << x.structure << " pairs=" << pairCount
              << " unpaired_fraction=" << unpairedFraction << " pair_indices=";
    for (size_t i = 0; i < pairIndices.size(); ++i) std::cout << (i ? "," : "") << pairIndices[i];
    std::cout << "\n";
    if (op.getNumOperands() == 2) {
      const auto slope = op->getAttrOfType<FloatAttr>("shape_slope")
                             ? op->getAttrOfType<FloatAttr>("shape_slope").getValueAsDouble()
                             : 1.8;
      const auto intercept = op->getAttrOfType<FloatAttr>("shape_intercept")
                                 ? op->getAttrOfType<FloatAttr>("shape_intercept").getValueAsDouble()
                                 : -0.6;
      std::cout << "constraint=shape-soft calibration=slope:" << slope << ",intercept:" << intercept << "\n";
    }
  });
  return ok ? 0 : 1;
}
