#pragma once
#include "mlir/IR/OpDefinition.h"
namespace ribo {
class KissingCandidateOp : public mlir::Op<KissingCandidateOp, mlir::OpTrait::ZeroResults, mlir::OpTrait::OneOperand> {
public:
  using Op::Op;
  static llvm::StringRef getOperationName() { return "ribo.kissing_candidate"; }
  static llvm::ArrayRef<llvm::StringRef> getAttributeNames() { static constexpr llvm::StringRef n[] = {"stem1_start", "stem1_end", "stem1_length", "stem2_start", "stem2_end", "stem2_length", "stem3_start", "stem3_end", "stem3_length", "loop1_min", "loop2_min", "loop3_min", "loop4_min", "loop5_min", "shared_stem", "shared_stem_score_once", "weight", "pair_score", "wobble_score"}; return n; }
  mlir::LogicalResult verify();
};
}
