// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "Heater.h"

namespace Clef::Fw {
Heater::Heater(TemperatureSensor &temperatureSensor,
               Clef::If::DirectOutputPwmTimer &pwmTimer,
               const DutyCycleSetter dutyCycleSetter, const float p,
               const float i, const float d)
    : pwmTimer_(pwmTimer),
      dutyCycleSetter_(dutyCycleSetter),
      pidController_(temperatureSensor, updateHeater, this, 0.0f, p, i, d, 1.0f,
                     0.1f) {}

void Heater::setTarget(const float target) { pidController_.setTarget(target); }

float Heater::getTarget() const { return pidController_.getTarget(); }

bool Heater::isAtTarget() const { return pidController_.isAtTarget(); }

void Heater::onLoop() { pidController_.onLoop(); }

void Heater::updateHeater(const float ratio, void *arg) {
  Heater *heater = reinterpret_cast<Heater *>(arg);
  (heater->pwmTimer_.*(heater->dutyCycleSetter_))(ratio);
}
}  // namespace Clef::Fw
