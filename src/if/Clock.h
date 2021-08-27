// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <util/Initialized.h>
#include <util/Units.h>

namespace Clef::If {
/**
 * Abstraction of a clock that can tell how many microseconds have passed since
 * the firmware started running.
 */
class Clock : public Clef::Util::Initialized {
 public:
  using Micros = Clef::Util::Time<uint64_t, Clef::Util::TimeUnit::USEC>;
  virtual Micros getMicros() const = 0;
};
}  // namespace Clef::If
