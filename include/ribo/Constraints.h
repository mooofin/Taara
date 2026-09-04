#pragma once

#include <string>
#include <vector>
#include "ribo/Runtime.h"

namespace ribo::runtime {

struct ShapeCalibration {
  double slope = 1.8;
  double intercept = -0.6;
};

[[nodiscard]] bool validShapeReactivities(const std::vector<double> &reactivities,
                                          std::string *error = nullptr);

[[nodiscard]] std::vector<double> shapePenalties(const std::vector<double> &reactivities,
                                                 ShapeCalibration calibration = {});

[[nodiscard]] FoldResult foldNestedWithShape(const std::string &sequence, int minLoop,
                               int canonicalScore,
                               const std::vector<double> &reactivities,
                               ShapeCalibration calibration = {});
[[nodiscard]] FoldResult foldHTypeWithShape(const std::string &sequence, int gap1MinLoop, int gap2MinLoop,
                               int gap3MinLoop, int stem1Length, int stem2Length, int canonicalScore,
                               const std::vector<double> &reactivities, ShapeCalibration calibration = {}, int wobbleScore = 0);

}
