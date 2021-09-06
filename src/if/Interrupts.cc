// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "Interrupts.h"

namespace Clef::If {
DisableInterrupts::DisableInterrupts() {
  if (areInterruptsEnabled()) {
    reenable_ = true;
    disableInterrupts();
  } else {
    reenable_ = false;
  }
}

DisableInterrupts::~DisableInterrupts() {
  if (reenable_) {
    enableInterrupts();
  }
}

EnableInterrupts::EnableInterrupts() {
  if (!areInterruptsEnabled()) {
    redisable_ = true;
    enableInterrupts();
  } else {
    redisable_ = false;
  }
}

EnableInterrupts::~EnableInterrupts() {
  if (redisable_) {
    disableInterrupts();
  }
}
}  // namespace Clef::If
