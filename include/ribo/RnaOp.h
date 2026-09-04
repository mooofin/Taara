#pragma once
#include "mlir/IR/OpDefinition.h"
namespace ribo {
class RnaOp : public mlir::Op<RnaOp, mlir::OpTrait::ZeroOperands, mlir::OpTrait::OneResult> {
public:
  using Op::Op;
  static llvm::StringRef getOperationName() { return "ribo.rna"; }
  static llvm::ArrayRef<llvm::StringRef> getAttributeNames() { static constexpr llvm::StringRef n[] = {"sequence"}; return n; }
  mlir::LogicalResult verify();
};
}
