// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <if/Clock.h>
#include <if/Serial.h>
#include <impl/atmega2560/PwmTimer.h>
#include <impl/atmega2560/Serial.h>

namespace Clef::Impl::Atmega2560 {
class Clock : public Clef::If::Clock {
 public:
  Clock(GenericTimer<uint16_t> &timer);
  bool init() override;
  Micros getMicros() const override;

 private:
  static void incrementNumMiddles(void *args);
  static void incrementNumEnds(void *args);

  GenericTimer<uint16_t> &timer_;
  uint32_t numMiddles_ = 0;
  uint32_t numEnds_ = 0;
};
}  // namespace Clef::Impl::Atmega2560
