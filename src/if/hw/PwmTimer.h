// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

namespace Clef::If::Hw {
/**
 * Abstraction of a timer that generates interrupts on its rising and falling
 * edges. It is parametrized in terms of frequency and duty-cycle. The timer can
 * be turned on and off so that it generates interrupts only some of the time.
 */
class PwmTimer {
 public:
  virtual void init() = 0;

  /**
   * Enable the timer to generate interrupts; the timer restarts its count.
   */
  virtual void enable() = 0;

  /**
   * Disable the timer from generating interrupts.
   */
  virtual void disable() = 0;

  virtual void setFrequency(const float frequency) = 0;
  virtual void setDutyCycle(const float dutyCycle) = 0;
  virtual void setRisingEdgeCb(void (*cb)(void *), void *data) = 0;
  virtual void setFallingEdgeCb(void (*cb)(void *), void *data) = 0;
};
}  // namespace Clef::If::Hw
