// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "Units.h"

namespace Clef::Util {
TimeSecs convertUsecsToSecs(const TimeUsecs usecs) {
  Time<float, TimeUnit::USEC> usecsFloat(*usecs);
  return TimeSecs(usecsFloat);
}
}  // namespace Clef::Util
