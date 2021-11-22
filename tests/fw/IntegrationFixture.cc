// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "IntegrationFixture.h"

namespace Clef::Fw {
IntegrationFixture::IntegrationFixture()
    : globalMutex_(std::make_shared<std::mutex>()),
      clock_(),
      serial_(globalMutex_),
      actionQueue_(),
      xyePositionQueue_(),
      parser_(),
      xAxisTimer_(),
      yAxisTimer_(),
      zAxisTimer_(),
      eAxisTimer_(),
      displacementSensorInput_(),
      displacementSensor_(clock_, 0.1),
      pressureSensor_(clock_, 0.02),
      extrusionPredictor_(0.2),
      syringeTemperatureSensor_(clock_, 10e3, 7.4e3),
      needleTemperatureSensor_(clock_, 10e3, 7.4e3),
      syringeHeater_(syringeTemperatureSensor_, sensingTimer_,
                     &Clef::If::DirectOutputPwmTimer::setDutyCycleA, 0.01f,
                     0.002f, 0.0f),
      needleHeater_(needleTemperatureSensor_, sensingTimer_,
                    &Clef::If::DirectOutputPwmTimer::setDutyCycleB, 0.01f,
                    0.002f, 0.0f),
      xAxis_(Clef::Impl::Emulator::xAxisStepper, xAxisTimer_),
      yAxis_(Clef::Impl::Emulator::yAxisStepper, yAxisTimer_),
      zAxis_(Clef::Impl::Emulator::zAxisStepper, zAxisTimer_),
      eAxis_(Clef::Impl::Emulator::eAxisStepper, eAxisTimer_,
             displacementSensor_, pressureSensor_, extrusionPredictor_,
             syringeHeater_, needleHeater_),
      axes_(xAxis_, yAxis_, zAxis_, eAxis_),
      context_(
          {axes_, parser_, clock_, serial_, actionQueue_, xyePositionQueue_}) {
  clock_.init();
  serial_.init();
  axes_.init();
  displacementSensorInput_.setConversionCallback(
      DisplacementSensor<USTEPS_PER_MM_DISPLACEMENT,
                         USTEPS_PER_MM_E>::injectWrapper,
      &displacementSensor_);
}
}  // namespace Clef::Fw
