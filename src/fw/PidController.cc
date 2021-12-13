// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "PidController.h"

namespace Clef::Fw {
PidController::PidController(Sensor<float> &sensor,
                             const OutputCallback outputCallback,
                             void *outputCallbackData, const float target,
                             const float pCoefficient, const float iCoefficient,
                             const float dCoefficient, const float maxOutput,
                             const float lowpassCoefficient)
    : sensor_(sensor),
      sensorToken_(sensor_.subscribe()),
      outputCallback_(outputCallback),
      outputCallbackData_(outputCallbackData),
      target_(target),
      pCoefficient_(pCoefficient),
      iCoefficient_(iCoefficient),
      dCoefficient_(dCoefficient),
      maxOutput_(maxOutput),
      lowpassCoefficient_(lowpassCoefficient),
      lastTime_(0),
      lastError_(0.0f),
      integral_(0.0f),
      derivative_(0.0f) {}

void PidController::reset(const float target,
                          const Clef::Util::TimeUsecs time) {
  target_ = target;
  lastTime_ = time;
  lastError_ = 0.0f;
  integral_ = 0.0f;
  derivative_ = 0.0f;
}

void PidController::onLoop() {
  if (sensor_.checkOut(sensorToken_)) {
    Clef::Fw::Sensor<float>::DataPoint dataPoint = sensor_.read();
    float error = dataPoint.data - target_;
    float timeDifference =
        *Clef::Util::convertUsecsToSecs(dataPoint.time - lastTime_);
    integral_ += 0.5 * (error + lastError_) * timeDifference;
    float derivative = (dataPoint.data - lastError_) / timeDifference;
    derivative_ = (1 - lowpassCoefficient_) * derivative_ +
                  lowpassCoefficient_ * derivative;
    lastError_ = error;
    lastTime_ = dataPoint.time;

    float pTerm = pCoefficient_ * error;
    float iTerm = iCoefficient_ * integral_;
    if (iTerm < -maxOutput_ / 2) {
      iTerm = -maxOutput_ / 2;
      integral_ = iTerm / iCoefficient_;
    } else if (iTerm > 0) {
      iTerm = 0;
      integral_ = maxOutput_ / 2 / iCoefficient_;
    }
    float dTerm = dCoefficient_ * derivative_;
    if (dTerm < -maxOutput_) {
      dTerm = -maxOutput_;
    } else if (dTerm > maxOutput_) {
      dTerm = maxOutput_;
    }
    float total = -(pTerm + iTerm + dTerm);
    if (total < 0) {
      total = 0;
    }
    if (total > 1) {
      total = 1;
    }
    if (outputCallback_) {
      outputCallback_(total, outputCallbackData_);
    }
    sensor_.release(sensorToken_);
  }
}

void PidController::setTarget(const float target) { target_ = target; }

float PidController::getTarget() const { return target_; }

bool PidController::isAtTarget() const { return lastError_ > 0.0f; }
}  // namespace Clef::Fw
