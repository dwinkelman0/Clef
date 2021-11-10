// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "ExtrusionPredictor.h"

#include <math.h>

namespace Clef::Fw {
void ExtrusionPredictor::reset(const float t, const float xe0,
                               const float xs0) {
  xe0_ = xe0;
  xs0_ = xs0;
  endpoint_ = 0.0f;
}

void ExtrusionPredictor::setEndpoint(const float endpoint) {
  endpoint_ = endpoint - xe0_;
}

float ExtrusionPredictor::getEndpoint() const { return endpoint_; }

bool ExtrusionPredictor::isBeyondEndpoint() const {
  return getRelativeExtrusionPosition() >= getEndpoint();
}

float ExtrusionPredictor::determineXYFeedrate(
    const float startX, const float startY, const float startE,
    const float endX, const float endY, const float endE, const float x,
    const float y) const {
  float xyProgress =
      sqrt((x - startX) * (x - startX) + (y - startY) * (y - startY));
  float xyTotal = sqrt((endX - startX) * (endX - startX) +
                       (endY - startY) * (endY - startY));
  float xyRatio = xyProgress / xyTotal;
  float eProgress = getRelativeExtrusionPosition() - (startE - xe0_);
  float eTotal = fabs(endE - startE);
  float eRatio = eProgress / eTotal;
  float xyLag = xyTotal * (eRatio - xyRatio);
  return getExtrusionRate() +
         xyLag * 30.0f; /*!< Eliminate half the lag per second. */
}

LinearExtrusionPredictor::LinearExtrusionPredictor(
    const float lowpassCoefficient)
    : lowpassCoefficient_(lowpassCoefficient) {}

void LinearExtrusionPredictor::reset(const float t, const float xe0,
                                     const float xs0) {
  ExtrusionPredictor::reset(t, xe0, xs0);
  t_ = t;
  xs_ = 0.0f;
  dxsdt_ = 0.0f;
}

void LinearExtrusionPredictor::evolve(const float t, const float xe,
                                      const float xs, const float P) {
  float xsNext = xs - xs0_;
  float dxsdtUpdate = dxsdt_ = (xsNext - xs_) / (t - t_) * 60;
  dxsdt_ =
      (1 - lowpassCoefficient_) * dxsdt_ + lowpassCoefficient_ * dxsdtUpdate;
  t_ = t;
  xs_ = xsNext;
}

float LinearExtrusionPredictor::getRelativeExtrusionPosition() const {
  return xs_;
}

float LinearExtrusionPredictor::getExtrusionRate() const { return dxsdt_; }

void KalmanFilterExtrusionPredictor::reset(const float t, const float xe0,
                                           const float xs0) {
  ExtrusionPredictor::reset(t, xe0, xs0);
  filter_.init();
  t_ = t;
}

void KalmanFilterExtrusionPredictor::evolve(const float t, const float xe,
                                            const float xs, const float P) {
  filter_.evolve(xe - xe0_, xs - xs0_, P, t - t_);
  t_ = t;
}

float KalmanFilterExtrusionPredictor::getRelativeExtrusionPosition() const {
  return filter_.getState().get(0, 0);
}

float KalmanFilterExtrusionPredictor::getExtrusionRate() const {
  return 60 * filter_.getState().get(1, 0);
}
}  // namespace Clef::Fw
