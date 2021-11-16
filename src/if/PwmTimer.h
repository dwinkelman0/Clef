// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <util/Initialized.h>
#include <util/Units.h>

namespace Clef::If {
class PwmTimerCommon : public Clef::Util::Initialized {
 public:
  /**
   * Enable the timer to generate interrupts; the timer restarts its count.
   */
  virtual void enable() = 0;

  /**
   * Disable the timer from generating interrupts.
   */
  virtual void disable() = 0;

  virtual bool isEnabled() const = 0;
};

/**
 * Abstraction of a timer that generates interrupts on its rising and falling
 * edges. It is parametrized in terms of frequency and duty-cycle. The timer can
 * be turned on and off so that it generates interrupts only some of the time.
 */
class PwmTimer : public PwmTimerCommon {
 public:
  using TransitionCallback = void (*)(void *);

  virtual void setFrequency(const Clef::Util::Frequency<float> frequency) = 0;
  virtual Clef::Util::Frequency<float> getMinFrequency() const = 0;
  virtual void setDutyCycle(const float dutyCycle) = 0;
  virtual void setRisingEdgeCallback(const TransitionCallback callback,
                                     void *data) = 0;
  virtual void setFallingEdgeCallback(const TransitionCallback callback,
                                      void *data) = 0;
};

class DirectOutputPwmTimer : public PwmTimerCommon {
 public:
  using TransitionCallback = void (*)(void *);

  virtual void setDutyCycleA(const float dutyCycle) = 0;
  virtual void setDutyCycleB(const float dutyCycle) = 0;
  virtual void setCallbackA(const TransitionCallback callback, void *data) = 0;
  virtual void setCallbackB(const TransitionCallback callback, void *data) = 0;
  virtual void setCallbackTop(const TransitionCallback callback,
                              void *data) = 0;
};
}  // namespace Clef::If
