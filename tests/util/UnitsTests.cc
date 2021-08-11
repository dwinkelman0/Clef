// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include <gtest/gtest.h>

#include "util/Units.h"

namespace Clef::Util {
TEST(Units, TimeConversions) {
  Time<int64_t, TimeUnit::MIN> mins(3);
  Time<int64_t, TimeUnit::SEC> secs(mins);
  Time<int64_t, TimeUnit::USEC> usecs(secs);
  ASSERT_EQ(*secs, 180);
  ASSERT_EQ(*usecs, 180000000);
  ASSERT_EQ(secs, (Time<int64_t, TimeUnit::SEC>(usecs)));
  ASSERT_EQ(mins, (Time<int64_t, TimeUnit::MIN>(usecs)));
  ASSERT_EQ(mins, (Time<int64_t, TimeUnit::MIN>(secs)));
}

TEST(Units, FrequencyConversions) {
  Time<float, TimeUnit::SEC> secs(0.001);
  Frequency<float> freq = secs.asFrequency();
  ASSERT_FLOAT_EQ(*freq, 1000.0f);
  ASSERT_FLOAT_EQ(*secs, *(freq.asTime()));
  Time<float, TimeUnit::USEC> usecs(1000);
  Frequency<float> freq2 =
      static_cast<Time<float, TimeUnit::SEC>>(usecs).asFrequency();
  ASSERT_FLOAT_EQ(*freq2, 1000.0f);
}

TEST(Units, PositionConversions) {
  Position<float, PositionUnit::MM, 400> mms(5);
  Position<float, PositionUnit::USTEP, 400> usteps(mms);
  ASSERT_EQ(*usteps, 2000);
  ASSERT_EQ(mms, (Position<float, PositionUnit::MM, 400>(usteps)));
}

TEST(Units, FeedrateConversions) {
  Feedrate<float, PositionUnit::MM, TimeUnit::MIN, 400> fr(1200);
  ASSERT_EQ(*fr, 1200);
  Time<float, TimeUnit::SEC> secs(Position<float, PositionUnit::MM, 400>(100) /
                                  fr);
  ASSERT_EQ(secs, (Time<float, TimeUnit::SEC>(5)));
  Position<float, PositionUnit::MM, 400> mms1(
      fr * Time<float, TimeUnit::MIN>(5.0f / 60));
  Position<float, PositionUnit::MM, 400> mms2(
      Time<float, TimeUnit::MIN>(5.0f / 60) * fr);
  ASSERT_FLOAT_EQ(*mms1, *mms2);
  ASSERT_FLOAT_EQ(*mms1, *(Position<float, PositionUnit::MM, 400>(100)));
}
}  // namespace Clef::Util
