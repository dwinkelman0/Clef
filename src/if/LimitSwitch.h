// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <stdint.h>
#include <util/Initialized.h>

namespace Clef::If {
class LimitSwitch : public Util::Initialized {
 public:
  using TriggerCallback = void (*)(void *, const uint8_t);

  LimitSwitch();

  /**
   * Move the limit switch out of the "triggered" state.
   */
  void reset();

  void setTriggerCallback(const TriggerCallback callback, void *data1,
                          const uint8_t data2);

  /**
   * Returns whether the limit switch is in the "triggered" state.
   */
  bool isTriggered() const;

 protected:
  /**
   * Call this function when the underlying digital input experiences a rising
   * or falling edge. If the underlying digital input indicates that the limit
   * switch is triggered, then the state of this limit switch changes to
   * "triggered". The callback only fires when the state changes to "triggered",
   * i.e. on the first rising edge since reset() was called.
   */
  void onTransition();

  /**
   * Returns true if the underlying digital input indicates the limit switch is
   * triggered.
   */
  virtual bool getInputState() const = 0;

  TriggerCallback triggerCallback_;
  void *triggerCallbackData1_;
  uint8_t triggerCallbackData2_;
  bool hasBeenTriggered_;
};
}  // namespace Clef::If
