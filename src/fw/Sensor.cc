// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "Sensor.h"

#include <math.h>

namespace Clef::Fw {
TemperatureSensor::TemperatureSensor(Clef::If::Clock &clock, const float Rt0,
                                     const float R0)
    : Sensor<float>(clock), Rratio_(R0 / Rt0) {}

void TemperatureSensor::injectWrapper(const float ratio, void *arg) {
  TemperatureSensor *sensor = reinterpret_cast<TemperatureSensor *>(arg);
  float normalizedR = sensor->Rratio_ * ratio / (1 - ratio);
  sensor->inject(convertNormalizedResistanceToTemperature(normalizedR));
}

float TemperatureSensor::convertNormalizedResistanceToTemperature(
    const float R) {
  if (R == 0) {
    return 999;
  }

  // These numbers are intrinsic to Amphenol Thermometrics Material Type 1
  const float a = 3.3539438e-3;
  const float b = 2.5646095e-4;
  const float c = 2.5158166e-6;
  const float d = 1.0503069e-7;
  float logR = log(R);
  return 1 / (a + logR * (b + logR * (c + d * logR))) - 273.15f;
}
}  // namespace Clef::Fw
