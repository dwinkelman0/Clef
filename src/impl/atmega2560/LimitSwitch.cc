// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "LimitSwitch.h"

#include <avr/io.h>
#include <if/Interrupts.h>

namespace Clef::Impl::Atmega2560 {
bool LimitSwitchBank::init() {
  Clef::If::DisableInterrupts noInterrupts;
  xLimitSwitch_.init();
  yLimitSwitch_.init();
  zLimitSwitch_.init();
  eIncLimitSwitch_.init();
  eDecLimitSwitch_.init();
  state_ = readState();
  PCICR |= 1 << PCIE2; /*!< Enable pin change interrupts. */
  PCMSK2 = 0x1f;       /*!< Enable pin change interrupts for specific pins. */
  return true;
}

void LimitSwitchBank::checkTransitions() {
  uint8_t newState = readState();
  uint8_t difference = newState ^ state_;
  xLimitSwitch_.checkForTransition(difference);
  yLimitSwitch_.checkForTransition(difference);
  zLimitSwitch_.checkForTransition(difference);
  eIncLimitSwitch_.checkForTransition(difference);
  eDecLimitSwitch_.checkForTransition(difference);
  state_ = newState;
}

uint8_t LimitSwitchBank::readState() const {
  return xLimitSwitch_.getInputStateVector() |
         yLimitSwitch_.getInputStateVector() |
         zLimitSwitch_.getInputStateVector() |
         eIncLimitSwitch_.getInputStateVector() |
         eDecLimitSwitch_.getInputStateVector();
}

ISR(PCINT2_vect) { limitSwitches.checkTransitions(); }

LimitSwitchBank limitSwitches;
}  // namespace Clef::Impl::Atmega2560
