// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "Stepper.h"

namespace Clef::Impl::Emulator {
bool XAxisStepperConfig::EnableRegister::state_ = false;
bool XAxisStepperConfig::DirectionRegister::state_ = false;
bool XAxisStepperConfig::PulseRegister::state_ = false;
bool XAxisStepperConfig::ResolutionRegister0::state_ = false;
bool XAxisStepperConfig::ResolutionRegister1::state_ = false;
bool XAxisStepperConfig::ResolutionRegister2::state_ = false;
bool YAxisStepperConfig::EnableRegister::state_ = false;
bool YAxisStepperConfig::DirectionRegister::state_ = false;
bool YAxisStepperConfig::PulseRegister::state_ = false;
bool YAxisStepperConfig::ResolutionRegister0::state_ = false;
bool YAxisStepperConfig::ResolutionRegister1::state_ = false;
bool YAxisStepperConfig::ResolutionRegister2::state_ = false;
bool ZAxisStepperConfig::EnableRegister::state_ = false;
bool ZAxisStepperConfig::DirectionRegister::state_ = false;
bool ZAxisStepperConfig::PulseRegister::state_ = false;
bool ZAxisStepperConfig::ResolutionRegister0::state_ = false;
bool ZAxisStepperConfig::ResolutionRegister1::state_ = false;
bool ZAxisStepperConfig::ResolutionRegister2::state_ = false;
bool EAxisStepperConfig::EnableRegister::state_ = false;
bool EAxisStepperConfig::DirectionRegister::state_ = false;
bool EAxisStepperConfig::PulseRegister::state_ = false;
bool EAxisStepperConfig::ResolutionRegister0::state_ = false;
bool EAxisStepperConfig::ResolutionRegister1::state_ = false;
bool EAxisStepperConfig::ResolutionRegister2::state_ = false;

XAxisStepper xAxisStepper;
YAxisStepper yAxisStepper;
ZAxisStepper zAxisStepper;
EAxisStepper eAxisStepper;
}  // namespace Clef::Impl::Emulator
