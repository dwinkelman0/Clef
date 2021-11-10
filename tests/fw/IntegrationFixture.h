// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include <fw/GcodeParser.h>
#include <gtest/gtest.h>
#include <impl/emulator/Clock.h>
#include <impl/emulator/PwmTimer.h>
#include <impl/emulator/SensorInput.h>
#include <impl/emulator/Serial.h>
#include <impl/emulator/Stepper.h>

namespace Clef::Fw {
class IntegrationFixture : public testing::Test {
 public:
  IntegrationFixture();

 protected:
  std::shared_ptr<std::mutex> globalMutex_;
  Clef::Impl::Emulator::Clock clock_;
  Clef::Impl::Emulator::Serial serial_;
  ActionQueue actionQueue_;
  XYEPositionQueue xyePositionQueue_;
  GcodeParser parser_;
  Clef::Impl::Emulator::GenericTimer xAxisTimer_;
  Clef::Impl::Emulator::GenericTimer yAxisTimer_;
  Clef::Impl::Emulator::GenericTimer zAxisTimer_;
  Clef::Impl::Emulator::GenericTimer eAxisTimer_;
  Clef::Impl::Emulator::DisplacementSensorInput displacementSensorInput_;
  DisplacementSensor<USTEPS_PER_MM_DISPLACEMENT, USTEPS_PER_MM_E>
      displacementSensor_;
  PressureSensor pressureSensor_;
  LinearExtrusionPredictor extrusionPredictor_;
  Axes::XAxis xAxis_;
  Axes::YAxis yAxis_;
  Axes::ZAxis zAxis_;
  Axes::EAxis eAxis_;
  Axes axes_;
  Context context_;
};
}  // namespace Clef::Fw
