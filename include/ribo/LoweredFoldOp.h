#pragma once
#include "mlir/IR/OpDefinition.h"
#include "ribo/Energy.h"
namespace ribo {
class LoweredFoldOp : public mlir::Op<LoweredFoldOp, mlir::OpTrait::ZeroOperands, mlir::OpTrait::ZeroResults> {
public:
  using Op::Op;
  static llvm::StringRef getOperationName() { return "ribo.lowered_fold"; }
  static llvm::ArrayRef<llvm::StringRef> getAttributeNames() { static constexpr llvm::StringRef n[] = {"topology", "strategy", "runtime", "state_rank", "state_variables", "dependencies", "storage", "stem_length", "stem1_length", "stem2_length", "gap1_min_loop", "gap2_min_loop", "gap3_min_loop", "time", "space", "optimality", "constraint", "shape_slope", "shape_intercept", "temperature_k", "sodium_m", "magnesium_m", "parameter_set"}; return n; }
  mlir::LogicalResult verify();
};
}
