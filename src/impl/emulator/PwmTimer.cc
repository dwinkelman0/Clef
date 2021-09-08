// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "PwmTimer.h"

namespace Clef::Impl::Emulator {

GenericTimer::GenericTimer() {}

bool GenericTimer::init() { return true; }

void GenericTimer::enable() { enabled_ = true; }

void GenericTimer::disable() { enabled_ = false; }

void GenericTimer::setFrequency(const Clef::Util::Frequency<float> frequency) {
  frequency_ = frequency;
}

Clef::Util::Frequency<float> GenericTimer::getMinFrequency() const { return 0; }

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

bool GenericTimer::isEnabled() const { return enabled_; }

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
}  // namespace Clef::Impl::Emulator
