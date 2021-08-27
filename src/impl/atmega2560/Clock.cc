// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "Clock.h"

#include <impl/atmega2560/Interrupts.h>

namespace Clef::Impl::Atmega2560 {
Clock::Clock(GenericTimer<uint16_t> &timer)
    : timer_(timer), numMiddles_(0), numEnds_(0) {}

bool Clock::init() {
  DisableInterrupts noInterrupts;
  timer_.init();
  timer_.setPrescaler(HardwareTimer<uint16_t>::Prescaling::_8);
  timer_.setCompareA(0xffff);
  timer_.setCompareB(0x7fff);
  timer_.setRisingEdgeCallback(incrementNumMiddles, this);
  timer_.setFallingEdgeCallback(incrementNumEnds, this);
  timer_.setCount(0);
  numMiddles_ = 0;
  numEnds_ = 0;
  timer_.enable();
  return true;
}

Clef::Util::Time<uint64_t, Clef::Util::TimeUnit::USEC> Clock::getMicros()
    const {
  DisableInterrupts noInterrupts;
  uint16_t count = timer_.getCount();
  uint32_t capturedValue = count < 0x7fff ? numMiddles_ : numEnds_;
  return (static_cast<uint64_t>(capturedValue) << 15) + (count >> 1);
}

void Clock::incrementNumMiddles(void *args) {
  Clock *clock = reinterpret_cast<Clock *>(args);
  clock->numMiddles_++;
}

void Clock::incrementNumEnds(void *args) {
  Clock *clock = reinterpret_cast<Clock *>(args);
  clock->numEnds_++;
}
}  // namespace Clef::Impl::Atmega2560
