// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include <gtest/gtest.h>
#include <impl/emulator/Clock.h>

#include <thread>

namespace Clef::Impl::Emulator {
TEST(ClockTest, Basic) {
  Clock clock;
  ASSERT_TRUE(clock.init());
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  ASSERT_GT(*clock.getMicros(),
            *(Clef::Util::Time<float, Clef::Util::TimeUnit::USEC>(10000)));
  ASSERT_LT(*clock.getMicros(),
            *(Clef::Util::Time<float, Clef::Util::TimeUnit::USEC>(100000)));
}
}  // namespace Clef::Impl::Emulator
