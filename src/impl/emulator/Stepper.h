// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <fw/Config.h>
#include <if/Stepper.h>
#include <impl/emulator/Register.h>

namespace Clef::Impl::Emulator {
class XAxisStepperConfig {
 public:
  class EnableRegister W_REGISTER_BOOL;
  class DirectionRegister W_REGISTER_BOOL;
  class PulseRegister W_REGISTER_BOOL;
  class ResolutionRegister0 W_REGISTER_BOOL;
  class ResolutionRegister1 W_REGISTER_BOOL;
  class ResolutionRegister2 W_REGISTER_BOOL;
};
class XAxisStepper
    : public Clef::If::StepperPartial<XAxisStepperConfig, USTEPS_PER_MM_X> {};
extern XAxisStepper xAxisStepper;

class YAxisStepperConfig {
 public:
  class EnableRegister W_REGISTER_BOOL;
  class DirectionRegister W_REGISTER_BOOL;
  class PulseRegister W_REGISTER_BOOL;
  class ResolutionRegister0 W_REGISTER_BOOL;
  class ResolutionRegister1 W_REGISTER_BOOL;
  class ResolutionRegister2 W_REGISTER_BOOL;
};
class YAxisStepper
    : public Clef::If::StepperPartial<YAxisStepperConfig, USTEPS_PER_MM_Y> {};
extern YAxisStepper yAxisStepper;

class ZAxisStepperConfig {
 public:
  class EnableRegister W_REGISTER_BOOL;
  class DirectionRegister W_REGISTER_BOOL;
  class PulseRegister W_REGISTER_BOOL;
  class ResolutionRegister0 W_REGISTER_BOOL;
  class ResolutionRegister1 W_REGISTER_BOOL;
  class ResolutionRegister2 W_REGISTER_BOOL;
};
class ZAxisStepper
    : public Clef::If::StepperPartial<ZAxisStepperConfig, USTEPS_PER_MM_Z> {};
extern ZAxisStepper zAxisStepper;

class EAxisStepperConfig {
 public:
  class EnableRegister W_REGISTER_BOOL;
  class DirectionRegister W_REGISTER_BOOL;
  class PulseRegister W_REGISTER_BOOL;
  class ResolutionRegister0 W_REGISTER_BOOL;
  class ResolutionRegister1 W_REGISTER_BOOL;
  class ResolutionRegister2 W_REGISTER_BOOL;
};
class EAxisStepper
    : public Clef::If::StepperPartial<EAxisStepperConfig, USTEPS_PER_MM_E> {};
extern EAxisStepper eAxisStepper;
}  // namespace Clef::Impl::Emulator
