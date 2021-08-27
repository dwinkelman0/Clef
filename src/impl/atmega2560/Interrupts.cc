// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "Interrupts.h"

#include <avr/interrupt.h>
#include <avr/io.h>

namespace Clef::Impl::Atmega2560 {
namespace {
bool areInterruptsEnabled() { return (SREG >> 7) & 1; }
}  // namespace

DisableInterrupts::DisableInterrupts() {
  if (areInterruptsEnabled()) {
    reenable_ = true;
    cli();
  } else {
    reenable_ = false;
  }
}

DisableInterrupts::~DisableInterrupts() {
  if (reenable_) {
    sei();
  }
}

EnableInterrupts::EnableInterrupts() {
  if (!areInterruptsEnabled()) {
    redisable_ = true;
    sei();
  } else {
    redisable_ = false;
  }
}

EnableInterrupts::~EnableInterrupts() {
  if (redisable_) {
    cli();
  }
}
}  // namespace Clef::Impl::Atmega2560
