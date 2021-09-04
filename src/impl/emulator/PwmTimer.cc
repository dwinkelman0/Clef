// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "PwmTimer.h"

namespace Clef::Impl::Emulator {

GenericTimer::GenericTimer(std::shared_ptr<std::mutex> globalMutex)
    : globalMutex_(globalMutex), frequency_(1.0f), loopIsActive_(false) {}

bool GenericTimer::init() { return true; }

void GenericTimer::enable() {
  if (loopFuture_.valid()) {
    loopFuture_.get();
  }
  loopFuture_ = std::async(std::launch::async, [this]() {
    loopIsActive_ = true;
    const Clef::Util::Time<float, Clef::Util::TimeUnit::USEC> period(
        frequency_.asTime());
    bool rising = true;
    while (loopIsActive_) {
      std::unique_lock<std::mutex> lock(*globalMutex_);
      if (rising) {
        if (risingEdgeCallback_) {
          risingEdgeCallback_(risingEdgeCallbackData_);
        }
      } else {
        if (fallingEdgeCallback_) {
          fallingEdgeCallback_(fallingEdgeCallbackData_);
        }
      }
      rising = !rising;
      std::this_thread::sleep_for(
          std::chrono::microseconds(static_cast<uint64_t>(*period / 2)));
    }
  });
}

void GenericTimer::disable() { loopIsActive_ = false; }

void GenericTimer::setFrequency(const Clef::Util::Frequency<float> frequency) {
  std::unique_lock<std::mutex> lock(*globalMutex_);
  frequency_ = frequency;
}

Clef::Util::Frequency<float> GenericTimer::getMinFrequency() const {
  return 0.001;
}

void GenericTimer::setRisingEdgeCallback(const TransitionCallback callback,
                                         void *data) {
  std::unique_lock<std::mutex> lock(*globalMutex_);
  risingEdgeCallback_ = callback;
  risingEdgeCallbackData_ = data;
}

void GenericTimer::setFallingEdgeCallback(const TransitionCallback callback,
                                          void *data) {
  std::unique_lock<std::mutex> lock(*globalMutex_);
  fallingEdgeCallback_ = callback;
  fallingEdgeCallbackData_ = data;
}
}  // namespace Clef::Impl::Emulator
