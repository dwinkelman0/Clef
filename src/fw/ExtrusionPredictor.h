// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <fw/kalman/Degen.h>
#include <util/Units.h>

namespace Clef::Fw {
class ExtrusionPredictor {
 public:
  /**
   * Reset the predictor to its initial state.
   */
  virtual void reset(const float t, const float xe0, const float xs0);

  /**
   * Set the target amount for the extrusion.
   */
  void setEndpoint(const float endpoint);
  float getEndpoint() const;

  /**
   * Check whether the target extrusion amount has been reached or exceeded.
   */
  bool isBeyondEndpoint() const;

  /**
   * Determine the feedrate for the XY direction.
   */
  float determineXYFeedrate(const float startX, const float startY,
                            const float startE, const float endX,
                            const float endY, const float endE, const float x,
                            const float y) const;

  /**
   * Evolve the internal stage of the predictor.
   *
   * Units are abandoned at this stage because prediction algorithms are
   * math-intensive and we want to avoid templating. Units are:
   *   - t: microseconds
   *   - xe, xs: E-axis usteps
   *   - P: pressure units
   */
  virtual void evolve(const float t, const float xe, const float xs,
                      const float P) = 0;

 protected:
  /**
   * Get the progress of the extrusion relative to the baseline xs0_.
   */
  virtual float getRelativeExtrusionPosition() const = 0;

  /**
   * Get the feedrate of the extrusion in E-axis usteps per minute.
   */
  virtual float getExtrusionRate() const = 0;

 protected:
  float endpoint_ = 0.0f; /*!< Extrusion endpoint relative to xe0_. */
  float xe0_ = 0.0f; /*!< xe is normalized against the position at reset. */
  float xs0_ = 0.0f; /*!< xs is normalized against the displacement at reset. */
};

/**
 * Uses simple linear extrapolation to predict extrusion rates.
 */
class LinearExtrusionPredictor : public ExtrusionPredictor {
 public:
  LinearExtrusionPredictor(const float lowpassCoefficient);

  void reset(const float t, const float xe0, const float xs0) override;

  void evolve(const float t, const float xe, const float xs,
              const float P) override;

 private:
  float getRelativeExtrusionPosition() const override;
  float getExtrusionRate() const override;

 private:
  float lowpassCoefficient_;
  float t_ = 0.0f;
  float xs_ = 0.0f;
  float dxsdt_ = 0.0f;
};

/**
 * Use a Kalman filter to represent the state of the extrusion system.
 */
class KalmanFilterExtrusionPredictor : public ExtrusionPredictor {
 public:
  void reset(const float t, const float xe0, const float xs0) override;

  void evolve(const float t, const float xe, const float xs,
              const float P) override;

 private:
  float getRelativeExtrusionPosition() const override;
  float getExtrusionRate() const override;

 private:
  Kalman::DegenFilter filter_;
  float t_ = 0.0f;
};
}  // namespace Clef::Fw
