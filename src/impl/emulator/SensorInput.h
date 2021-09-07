// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <fw/Config.h>
#include <if/Clock.h>
#include <if/SensorInput.h>
#include <util/Units.h>

namespace Clef::Impl::Emulator {
namespace {
using Position = Clef::Util::Position<float, Clef::Util::PositionUnit::MM,
                                      USTEPS_PER_MM_DISPLACEMENT>;
}

class DisplacementSensorInput : public Clef::If::SensorInput<Position> {
 public:
  bool init() override { return true; }

  void inject(const Position data) {
    if (conversionCallback_) {
      conversionCallback_(data, conversionCallbackData_);
    }
  }
};
}  // namespace Clef::Impl::Emulator
