// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

namespace Clef::Impl::Atmega2560 {
/**
 * Create an instance of this object to disable interrupts (regardless of
 * current SREG state) for the lifetime of this object; it restores SREG to its
 * old state upon destruction.
 */
class DisableInterrupts {
 public:
  DisableInterrupts();
  ~DisableInterrupts();

 private:
  bool reenable_; /*!< Keep track of whether interrupts need to be re-enabled.
                   */
};

/**
 * Create an instance of this object to enable interrupts (regardless of
 * current SREG state) for the lifetime of this object; it restores SREG to its
 * old state upon destruction.
 */
class EnableInterrupts {
 public:
  EnableInterrupts();
  ~EnableInterrupts();

 private:
  bool redisable_; /*!< Keep track of whether interrupts need to be re-disabled.
                    */
};
}  // namespace Clef::Impl::Atmega2560
