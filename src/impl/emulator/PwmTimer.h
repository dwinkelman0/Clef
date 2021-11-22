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

class GenericDirectOutputTimer : public Clef::If::DirectOutputPwmTimer {
 public:
  bool init() override;
  void enable() override;
  void disable() override;
  bool isEnabled() const override;
  void setDutyCycleA(const float dutyCycle) override;
  void setDutyCycleB(const float dutyCycle) override;
  float getDutyCycleA() const override;
  float getDutyCycleB() const override;
  void setCallbackA(const TransitionCallback callback, void *data) override;
  void setCallbackB(const TransitionCallback callback, void *data) override;
  void setCallbackTop(const TransitionCallback callback, void *data) override;

 private:
  float dutyCycleA_ = 0.0f;
  TransitionCallback callbackA_ = nullptr;
  void *callbackAData_ = nullptr;
  float dutyCycleB_ = 0.0f;
  TransitionCallback callbackB_ = nullptr;
  void *callbackBData_ = nullptr;
  TransitionCallback callbackTop_ = nullptr;
  void *callbackTopData_ = nullptr;
  bool enabled_ = false;
};
}  // namespace Clef::Impl::Emulator
