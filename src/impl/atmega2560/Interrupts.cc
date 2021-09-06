// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include <avr/interrupt.h>
#include <avr/io.h>
#include <if/Interrupts.h>

namespace Clef::If {
bool areInterruptsEnabled() { return (SREG >> 7) & 1; }

void disableInterrupts() { cli(); }

void enableInterrupts() { sei(); }
}  // namespace Clef::If
