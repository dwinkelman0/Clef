// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "Clock.h"

namespace Clef::Impl::Emulator {
bool Clock::init() {
  t0_ = std::chrono::high_resolution_clock::now();
  return true;
}

Clef::Util::Time<uint64_t, Clef::Util::TimeUnit::USEC> Clock::getMicros()
    const {
  auto elapsed = std::chrono::high_resolution_clock::now() - t0_;
  return Clef::Util::Time<uint64_t, Clef::Util::TimeUnit::USEC>(
      std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count());
}
};  // namespace Clef::Impl::Emulator
