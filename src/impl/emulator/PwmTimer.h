// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <if/PwmTimer.h>

#include <functional>
#include <future>
#include <memory>
#include <mutex>

namespace Clef::Impl::Emulator {
class GenericTimer : public Clef::If::PwmTimer {
 public:
  GenericTimer();
  bool init() override;
  void enable() override;
  void disable() override;
  bool isEnabled() const override;
  void setFrequency(const Clef::Util::Frequency<float> frequency) override;
  Clef::Util::Frequency<float> getMinFrequency() const override;
  void setDutyCycle(const float dutyCycle) override;
  void setRisingEdgeCallback(const TransitionCallback callback,
                             void *data) override;
  void setFallingEdgeCallback(const TransitionCallback callback,
                              void *data) override;

  Clef::Util::Frequency<float> getFrequency() const;
  void pulseOnce() const;
  void pulseWhile(const std::function<bool(void)> &predicate) const;

 private:
  TransitionCallback risingEdgeCallback_ = nullptr;
  void *risingEdgeCallbackData_ = nullptr;
  TransitionCallback fallingEdgeCallback_ = nullptr;
  void *fallingEdgeCallbackData_ = nullptr;
  Clef::Util::Frequency<float> frequency_ = 1.0f;
  bool enabled_ = false;
};
}  // namespace Clef::Impl::Emulator
