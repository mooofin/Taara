#pragma once
#include "mlir/IR/OpDefinition.h"
namespace ribo {
class ConstraintOp : public mlir::Op<ConstraintOp, mlir::OpTrait::OneResult, mlir::OpTrait::OneOperand> {
public:
  using Op::Op;
  static llvm::StringRef getOperationName() { return "ribo.constraint"; }
  static llvm::ArrayRef<llvm::StringRef> getAttributeNames() { static constexpr llvm::StringRef n[] = {"kind", "reactivity"}; return n; }
  mlir::LogicalResult verify();
};
}
