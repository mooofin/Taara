#include "ribo/Energy.h"
#include <cmath>

namespace ribo::energy {
bool valid(const EnergyContext &context, std::string *error) {
  const auto fail = [&](const char *message) { if (error) *error = message; return false; };
  if (!std::isfinite(context.temperatureK) || context.temperatureK <= 0.0) return fail("temperatureK must be finite and positive");
  if (!std::isfinite(context.sodiumM) || context.sodiumM < 0.0) return fail("sodiumM must be finite and non-negative");
  if (!std::isfinite(context.magnesiumM) || context.magnesiumM < 0.0) return fail("magnesiumM must be finite and non-negative");
  if (context.parameterSet.empty()) return fail("parameterSet must be non-empty");
  return true;
}
}
