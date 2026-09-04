#include "ribo/ConstraintOp.h"
#include "ribo/RnaOp.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/BuiltinTypes.h"
using namespace mlir;
namespace ribo {
LogicalResult ConstraintOp::verify() {
  auto kind = (*this)->getAttrOfType<StringAttr>("kind"); if (!kind || kind.getValue() != "shape") return emitError("constraint kind must be \"shape\"");
  auto values = (*this)->getAttrOfType<DenseFPElementsAttr>("reactivity"); if (!values) return emitError("shape constraint requires dense f32 reactivity values");
  auto result = llvm::dyn_cast<RankedTensorType>((*this)->getResult(0).getType()); if (!result || result.getRank() != 1 || !result.getElementType().isF32()) return emitError("constraint result must be a rank-1 f32 tensor");
  for (const auto &value : values.getValues<APFloat>()) if (!value.isFinite() || value.isNegative()) return emitError("SHAPE reactivities must be finite and non-negative");
  if (auto rna = getOperand().getDefiningOp<RnaOp>()) if (auto sequence = rna->getAttrOfType<StringAttr>("sequence")) if (values.getNumElements() != sequence.getValue().size()) return emitError("SHAPE reactivity length must match RNA sequence");
  return success();
}
}
