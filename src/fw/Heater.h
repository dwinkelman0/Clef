// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <fw/PidController.h>
#include <fw/Sensor.h>
#include <if/PwmTimer.h>

namespace Clef::Fw {
class Heater {
 public:
  using DutyCycleSetter = void (Clef::If::DirectOutputPwmTimer::*)(float);

  Heater(TemperatureSensor &temperatureSensor,
         Clef::If::DirectOutputPwmTimer &pwmTimer,
         const DutyCycleSetter dutyCycleSetter, const float p, const float i,
         const float d);

  void setTarget(const float target);
  float getTarget() const;
  bool isAtTarget() const;
  void onLoop();

 private:
  static void updateHeater(const float ratio, void *arg);

  Clef::If::DirectOutputPwmTimer &pwmTimer_;
  DutyCycleSetter dutyCycleSetter_;
  PidController pidController_;
};
}  // namespace Clef::Fw
