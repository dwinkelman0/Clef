// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "LimitSwitch.h"

#include <if/Interrupts.h>

namespace Clef::If {
LimitSwitch::LimitSwitch()
    : triggerCallback_(nullptr),
      triggerCallbackData_(nullptr),
      hasBeenTriggered_(false) {}

void LimitSwitch::reset() { hasBeenTriggered_ = getInputState(); }

void LimitSwitch::setTriggerCallback(const TriggerCallback callback,
                                     void *data) {
  triggerCallback_ = callback;
  triggerCallbackData_ = data;
}

bool LimitSwitch::isTriggered() const {
  return hasBeenTriggered_ || getInputState();
}

void LimitSwitch::onTransition() {
  if (!hasBeenTriggered_ && getInputState()) {
    hasBeenTriggered_ = true;
    if (triggerCallback_) {
      triggerCallback_(triggerCallbackData_);
    }
  }
}
}  // namespace Clef::If
