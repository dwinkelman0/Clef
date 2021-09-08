// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "SensorInput.h"

namespace Clef::Impl::Atmega2560 {
RINT_REGISTER_BOOL_ISRS(ExtruderCaliperConfig::ClockRegister, 4);
ExtruderCaliper extruderCaliper;
}  // namespace Clef::Impl::Atmega2560
