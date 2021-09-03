// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <if/PwmTimer.h>

#include <future>
#include <memory>
#include <mutex>

namespace Clef::Impl::Emulator {
class GenericTimer : public Clef::If::PwmTimer {
 public:
  GenericTimer(std::shared_ptr<std::mutex> globalMutex);
  bool init() override;
  void enable() override;
  void disable() override;
  void setFrequency(const Clef::Util::Frequency<float> frequency) override;
  void setRisingEdgeCallback(const TransitionCallback callback,
                             void *data) override;
  void setFallingEdgeCallback(const TransitionCallback callback,
                              void *data) override;

 private:
  std::shared_ptr<std::mutex> globalMutex_;
  TransitionCallback risingEdgeCallback_ = nullptr;
  void *risingEdgeCallbackData_ = nullptr;
  TransitionCallback fallingEdgeCallback_ = nullptr;
  void *fallingEdgeCallbackData_ = nullptr;
  Clef::Util::Frequency<float> frequency_;
  std::future<void> loopFuture_;
  bool loopIsActive_;
};
}  // namespace Clef::Impl::Emulator
