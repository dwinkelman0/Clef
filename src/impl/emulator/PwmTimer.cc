// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "PwmTimer.h"

namespace Clef::Impl::Emulator {

bool GenericTimer::init() { return true; }

void GenericTimer::enable() { enabled_ = true; }

void GenericTimer::disable() { enabled_ = false; }

bool GenericTimer::isEnabled() const { return enabled_; }

void GenericTimer::setFrequency(const Clef::Util::Frequency<float> frequency) {
  frequency_ = frequency;
}

Clef::Util::Frequency<float> GenericTimer::getMinFrequency() const { return 0; }

void GenericTimer::setDutyCycle(const float dutyCycle) {}

void GenericTimer::setRisingEdgeCallback(const TransitionCallback callback,
                                         void *data) {
  risingEdgeCallback_ = callback;
  risingEdgeCallbackData_ = data;
}

void GenericTimer::setFallingEdgeCallback(const TransitionCallback callback,
                                          void *data) {
  fallingEdgeCallback_ = callback;
  fallingEdgeCallbackData_ = data;
}

Clef::Util::Frequency<float> GenericTimer::getFrequency() const {
  return frequency_;
}

void GenericTimer::pulseOnce() const {
  if (enabled_) {
    if (risingEdgeCallback_) {
      risingEdgeCallback_(risingEdgeCallbackData_);
    }
    if (fallingEdgeCallback_) {
      fallingEdgeCallback_(fallingEdgeCallbackData_);
    }
  }
}

void GenericTimer::pulseWhile(
    const std::function<bool(void)> &predicate) const {
  while (predicate()) {
    if (enabled_) {
      if (risingEdgeCallback_) {
        risingEdgeCallback_(risingEdgeCallbackData_);
      }
      if (fallingEdgeCallback_) {
        fallingEdgeCallback_(fallingEdgeCallbackData_);
      }
    }
  }
}

bool GenericDirectOutputTimer::init() { return true; }

void GenericDirectOutputTimer::enable() { enabled_ = true; }

void GenericDirectOutputTimer::disable() { enabled_ = false; }

bool GenericDirectOutputTimer::isEnabled() const { return enabled_; }

void GenericDirectOutputTimer::setDutyCycleA(const float dutyCycle) {
  dutyCycleA_ = dutyCycle;
}

void GenericDirectOutputTimer::setDutyCycleB(const float dutyCycle) {
  dutyCycleB_ = dutyCycle;
}

float GenericDirectOutputTimer::getDutyCycleA() const { return dutyCycleA_; }

float GenericDirectOutputTimer::getDutyCycleB() const { return dutyCycleB_; }

void GenericDirectOutputTimer::setCallbackA(const TransitionCallback callback,
                                            void *data) {
  callbackA_ = callback;
  callbackAData_ = data;
}

void GenericDirectOutputTimer::setCallbackB(const TransitionCallback callback,
                                            void *data) {
  callbackB_ = callback;
  callbackBData_ = data;
}

void GenericDirectOutputTimer::setCallbackTop(const TransitionCallback callback,
                                              void *data) {
  callbackTop_ = callback;
  callbackTopData_ = data;
}
}  // namespace Clef::Impl::Emulator
