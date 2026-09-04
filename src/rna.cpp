#include "ribo/RnaOp.h"
#include "mlir/IR/BuiltinAttributes.h"
using namespace mlir;
namespace ribo {
LogicalResult RnaOp::verify() {
  auto sequence = (*this)->getAttrOfType<StringAttr>("sequence"); if (!sequence || sequence.getValue().empty()) return emitError("RNA sequence must be non-empty");
  for (char c : sequence.getValue()) { if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A'); if (c != 'A' && c != 'C' && c != 'G' && c != 'U') return emitError("RNA sequence must contain only A, C, G, and U"); }
  return success();
}
}
