// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "ExtrusionPredictor.h"

#include <math.h>

namespace Clef::Fw {
void LinearExtrusionPredictor::reset(const float xe0, const float xs0) {
  endpoint_ = 0.0f;
  xe_ = 0.0f;
  xe0_ = xe0;
  xs_ = 0.0f;
  xs0_ = xs0;
  dxsdt_ = 0.0f;
}

void LinearExtrusionPredictor::setEndpoint(const float endpoint) {
  endpoint_ = endpoint - xe0_;
}

float LinearExtrusionPredictor::getEndpoint() const { return endpoint_ + xe0_; }

void LinearExtrusionPredictor::evolve(const float t, const float xe,
                                      const float xs, const float dxsdt,
                                      const float P) {
  xe_ = xe - xe0_;
  xs_ = xs - xs0_;
  dxsdt_ = dxsdt;
}

bool LinearExtrusionPredictor::isBeyondEndpoint() const {
  return xs_ >= endpoint_;
}

float LinearExtrusionPredictor::determineExtruderTargetPosition() const {
  float difference = endpoint_ - xs_;
  if (difference > 500) {
    return endpoint_ + 1000;
  } else if (difference > 50) {
    return endpoint_ + 2 * difference;
  } else {
    return endpoint_;
  }
}

float LinearExtrusionPredictor::determineExtruderFeedrate() const {
  return 40.0f;
}

float LinearExtrusionPredictor::determineXYFeedrate(
    const float startX, const float startY, const float startE,
    const float endX, const float endY, const float endE, const float x,
    const float y) const {
  float xyProgress =
      sqrt((x - startX) * (x - startX) + (y - startY) * (y - startY));
  float xyTotal = sqrt((endX - startX) * (endX - startX) +
                       (endY - startY) * (endY - startY));
  float xyRatio = xyProgress / xyTotal;
  float eProgress = fabs(xs_ - (startE - xe0_));
  float eTotal = fabs(endE - startE);
  float eRatio = eProgress / eTotal;
  float xyLag = xyTotal * (eRatio - xyRatio);
  return dxsdt_ + xyLag * 30.0f; /*!< Eliminate half the lag per second. */
}
}  // namespace Clef::Fw
