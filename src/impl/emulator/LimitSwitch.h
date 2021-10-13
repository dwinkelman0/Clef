// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <if/LimitSwitch.h>

namespace Clef::Impl::Emulator {
class LimitSwitch : public Clef::If::LimitSwitch {
 public:
  LimitSwitch() : state_(false) {}

  bool init() override { return true; }

  bool getInputState() const override { return state_; }

  void setInputState(const bool state) {
    if (state_ != state) {
      state_ = state;
      onTransition();
    }
  }

 private:
  bool state_;
};
}  // namespace Clef::Impl::Emulator
