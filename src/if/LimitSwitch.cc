// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "LimitSwitch.h"

#include <if/Interrupts.h>

namespace Clef::If {
LimitSwitch::LimitSwitch()
    : triggerCallback_(nullptr),
      triggerCallbackData1_(nullptr),
      triggerCallbackData2_(0),
      hasBeenTriggered_(false) {}

void LimitSwitch::reset() { hasBeenTriggered_ = getInputState(); }

void LimitSwitch::setTriggerCallback(const TriggerCallback callback,
                                     void *data1, const uint8_t data2) {
  triggerCallback_ = callback;
  triggerCallbackData1_ = data1;
  triggerCallbackData2_ = data2;
}

bool LimitSwitch::isTriggered() const {
  return hasBeenTriggered_ || getInputState();
}

void LimitSwitch::onTransition() {
  if (!hasBeenTriggered_ && getInputState()) {
    hasBeenTriggered_ = true;
    if (triggerCallback_) {
      triggerCallback_(triggerCallbackData1_, triggerCallbackData2_);
    }
  }
}
}  // namespace Clef::If
