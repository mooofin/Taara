#pragma once
#include <string>

namespace ribo::energy {
struct EnergyContext { double temperatureK = 310.15, sodiumM = 0.0, magnesiumM = 0.0; std::string parameterSet; };
[[nodiscard]] bool valid(const EnergyContext &context, std::string *error = nullptr);
}
