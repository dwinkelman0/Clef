// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <util/Units.h>

namespace Clef::Fw {
class ExtrusionPredictor {
 public:
  /**
   * Reset the predictor to its initial state.
   */
  virtual void reset(const float xe0, const float xs0) = 0;

  /**
   * Set the target amount for the extrusion.
   */
  virtual void setEndpoint(const float endpoint) = 0;
  virtual float getEndpoint() const = 0;

  /**
   * Evolve the internal stage of the predictor.
   *
   * Units are abandoned at this stage because prediction algorithms are
   * math-intensive and we want to avoid templating. Units are:
   *   - t: microseconds
   *   - xe, xs: E-axis usteps
   *   - dxsdt: E-axis usteps per minute
   *   - P: pressure units
   */
  virtual void evolve(const float t, const float xe, const float xs,
                      const float dxsdt, const float P) = 0;

  /**
   * Check whether the target extrusion amount has been reached or exceeded.
   */
  virtual bool isBeyondEndpoint() const = 0;

  /**
   * Determine the target position for the underlying physical actuator.
   */
  virtual float determineExtruderTargetPosition() const = 0;

  /**
   * Determine the feedrate for the underlying physical actuator.
   */
  virtual float determineExtruderFeedrate() const = 0;

  /**
   * Determine the feedrate for the XY direction.
   */
  virtual float determineXYFeedrate(const float startX, const float startY,
                                    const float startE, const float endX,
                                    const float endY, const float endE,
                                    const float x, const float y) const = 0;
};

/**
 * Uses simple linear extrapolation to predict extrusion rates.
 */
class LinearExtrusionPredictor : public ExtrusionPredictor {
 public:
  void reset(const float xe0, const float xs0) override;
  void setEndpoint(const float endpoint) override;
  float getEndpoint() const override;
  void evolve(const float t, const float xe, const float xs, const float dxsdt,
              const float P) override;
  bool isBeyondEndpoint() const override;
  float determineExtruderTargetPosition() const override;
  float determineExtruderFeedrate() const override;
  float determineXYFeedrate(const float startX, const float startY,
                            const float startE, const float endX,
                            const float endY, const float endE, const float x,
                            const float y) const override;

 private:
  float endpoint_ = 0.0f;
  float xe_ = 0.0f;
  float xe0_ = 0.0f; /*!< xe is normalized against the position at reset. */
  float xs_ = 0.0f;
  float xs0_ = 0.0f; /*!< xs is normalized against the displacement at reset. */
  float dxsdt_ = 0.0f;
};
}  // namespace Clef::Fw
