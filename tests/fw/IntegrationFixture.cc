// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "IntegrationFixture.h"

namespace Clef::Fw {
IntegrationFixture::IntegrationFixture()
    : globalMutex_(std::make_shared<std::mutex>()),
      clock_(),
      serial_(globalMutex_),
      actionQueue_(),
      parser_(),
      xAxisTimer_(globalMutex_),
      yAxisTimer_(globalMutex_),
      zAxisTimer_(globalMutex_),
      eAxisTimer_(globalMutex_),
      displacementSensorInput_(),
      displacementSensor_(clock_, 0.1),
      xAxis_(Clef::Impl::Emulator::xAxisStepper, xAxisTimer_),
      yAxis_(Clef::Impl::Emulator::yAxisStepper, yAxisTimer_),
      zAxis_(Clef::Impl::Emulator::zAxisStepper, zAxisTimer_),
      eAxis_(Clef::Impl::Emulator::eAxisStepper, eAxisTimer_,
             displacementSensor_),
      axes_(xAxis_, yAxis_, zAxis_, eAxis_),
      context_({axes_, parser_, clock_, serial_, actionQueue_}) {
  clock_.init();
  serial_.init();
  displacementSensorInput_.setConversionCallback(
      DisplacementSensor<USTEPS_PER_MM_DISPLACEMENT,
                         USTEPS_PER_MM_E>::injectWrapper,
      &displacementSensor_);
}
}  // namespace Clef::Fw
