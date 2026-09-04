#include "ribo/LoweredFoldOp.h"
#include "mlir/IR/BuiltinAttributes.h"

namespace ribo {
mlir::LogicalResult LoweredFoldOp::verify() {
  const bool hasEnergy = (*this)->getAttr("temperature_k") || (*this)->getAttr("sodium_m") || (*this)->getAttr("magnesium_m") || (*this)->getAttr("parameter_set");
  if (!hasEnergy) return mlir::success();
  const auto temperature = (*this)->getAttrOfType<mlir::FloatAttr>("temperature_k");
  const auto sodium = (*this)->getAttrOfType<mlir::FloatAttr>("sodium_m");
  const auto magnesium = (*this)->getAttrOfType<mlir::FloatAttr>("magnesium_m");
  const auto parameters = (*this)->getAttrOfType<mlir::StringAttr>("parameter_set");
  if (!temperature || !sodium || !magnesium || !parameters) return emitError("thermodynamic metadata requires temperature_k, sodium_m, magnesium_m, and parameter_set");
  std::string error;
  if (!energy::valid({temperature.getValueAsDouble(), sodium.getValueAsDouble(), magnesium.getValueAsDouble(), parameters.getValue().str()}, &error)) return emitError() << error;
  return mlir::success();
}
}
