// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "Sensor.h"

#include <if/Interrupts.h>
#include <math.h>
#include <string.h>

namespace Clef::Fw {
TemperatureSensor::TemperatureSensor(Clef::If::Clock &clock, const float Rt0,
                                     const float R0)
    : Sensor<float>(clock), Rratio_(R0 / Rt0) {}

void TemperatureSensor::injectWrapper(const float ratio, void *arg) {
  Clef::If::EnableInterrupts enableInterrupts;
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

MassSensor::MassSensor(Clef::If::Clock &clock, Clef::If::RSerial &serial)
    : Sensor<int32_t>(clock), serial_(serial) {
  reset();
}

void MassSensor::ingest(Clef::If::RWSerial &debugSerial) {
  const uint16_t errorBufferSize = 64;
  char errorBuffer[errorBufferSize];
  char newChar;
  while (serial_.read(&newChar)) {
    if (newChar == '\n') {
      if (!parse()) {
        debugSerial.writeStr(";mass_sensor_parse_error: ");
        debugSerial.writeLine(buffer_);
      }
      reset();
    } else if (!append(newChar)) {
      reset();
    }
  }
}

void MassSensor::reset() {
  memset(buffer_, 0, sizeof(buffer_));
  head_ = buffer_;
}

bool MassSensor::append(const char newChar) {
  if (head_ < (buffer_ + size_ - 1)) {
    *(head_++) = newChar;
    *head_ = '\0';
    return true;
  }
  return false;
}

bool MassSensor::parse() {
  if ((buffer_[0] != '-' && buffer_[0] != '+') ||
      !(buffer_[10] == 'g' && buffer_[11] == ' ' && buffer_[12] == ' ')) {
    return false;
  }
  bool minusSign = buffer_[0] == '-';
  int32_t total = 0;
  for (int i = 1; i < 10; ++i) {
    if (buffer_[i] == '.' || buffer_[i] == ' ') {
      continue;
    }
    total *= 10;
    total += buffer_[i] - '0';
  }
  if (minusSign) {
    total = -total;
  }
  inject(total);
  return true;
}
}  // namespace Clef::Fw
