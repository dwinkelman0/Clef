// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <fw/Sensor.h>
#include <util/Initialized.h>

namespace Clef::Fw {
class PidController {
 public:
  /**
   * The output callback is called whenever an update to the output device
   * should be made. The first parameter is a value in [0, 1].
   */
  using OutputCallback = void (*)(const float, void *);

  PidController(Sensor<float> &sensor, const OutputCallback outputCallback,
                void *outputCallbackData, const float target,
                const float pCoefficient, const float iCoefficient,
                const float dCoefficient, const float maxOutput,
                const float lowpassCoefficient);

  void reset(const float target, const Clef::Util::TimeUsecs time);
  void onLoop();
  void setTarget(const float target);
  float getTarget() const;
  bool isAtTarget() const;

 private:
  Sensor<float> &sensor_;
  uint8_t sensorToken_;
  OutputCallback outputCallback_;
  void *outputCallbackData_;
  float target_;
  float pCoefficient_;
  float iCoefficient_;
  float dCoefficient_;
  float maxOutput_;
  float lowpassCoefficient_;
  Clef::Util::TimeUsecs lastTime_;
  float lastError_;
  float integral_;
  float derivative_;
};
}  // namespace Clef::Fw
