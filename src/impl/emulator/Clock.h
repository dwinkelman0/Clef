// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <if/Clock.h>

#include <chrono>

namespace Clef::Impl::Emulator {
class Clock : public Clef::If::Clock {
 public:
  bool init() override;
  Clef::Util::Time<uint64_t, Clef::Util::TimeUnit::USEC> getMicros()
      const override;

 private:
  std::chrono::time_point<std::chrono::high_resolution_clock> t0_;
};
}  // namespace Clef::Impl::Emulator
