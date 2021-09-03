// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <if/Stepper.h>
#include <impl/atmega2560/Config.h>
#include <impl/atmega2560/Register.h>

namespace Clef::Impl::Atmega2560 {
class XAxisStepperConfig {
 public:
  class EnableRegister WREGISTER_BOOL(L, 1, true);       /*!< Pin 48. */
  class DirectionRegister WREGISTER_BOOL(D, 7, false);   /*!< Pin 38. */
  class PulseRegister WREGISTER_BOOL(G, 1, false);       /*!< Pin 40. */
  class ResolutionRegister0 WREGISTER_BOOL(L, 3, false); /*!< Pin 46. */
  class ResolutionRegister1 WREGISTER_BOOL(L, 5, false); /*!< Pin 44. */
  class ResolutionRegister2 WREGISTER_BOOL(L, 7, false); /*!< Pin 42. */
};
class XAxisStepper
    : public Clef::If::StepperPartial<XAxisStepperConfig, USTEPS_PER_MM_X> {};
extern XAxisStepper xAxisStepper;

class YAxisStepperConfig {
 public:
  class EnableRegister WREGISTER_BOOL(C, 1, true);       /*!< Pin 36. */
  class DirectionRegister WREGISTER_BOOL(A, 4, false);   /*!< Pin 26. */
  class PulseRegister WREGISTER_BOOL(A, 6, false);       /*!< Pin 28. */
  class ResolutionRegister0 WREGISTER_BOOL(C, 3, false); /*!< Pin 34. */
  class ResolutionRegister1 WREGISTER_BOOL(C, 5, false); /*!< Pin 32. */
  class ResolutionRegister2 WREGISTER_BOOL(C, 7, false); /*!< Pin 30. */
};
class YAxisStepper
    : public Clef::If::StepperPartial<YAxisStepperConfig, USTEPS_PER_MM_Y> {};
extern YAxisStepper yAxisStepper;

class ZAxisStepperConfig {
 public:
  class EnableRegister WREGISTER_BOOL(C, 0, true);       /*!< Pin 37. */
  class DirectionRegister WREGISTER_BOOL(A, 5, false);   /*!< Pin 27. */
  class PulseRegister WREGISTER_BOOL(A, 7, false);       /*!< Pin 29. */
  class ResolutionRegister0 WREGISTER_BOOL(C, 2, false); /*!< Pin 35. */
  class ResolutionRegister1 WREGISTER_BOOL(C, 4, false); /*!< Pin 33. */
  class ResolutionRegister2 WREGISTER_BOOL(C, 6, false); /*!< Pin 31. */
};
class ZAxisStepper
    : public Clef::If::StepperPartial<ZAxisStepperConfig, USTEPS_PER_MM_Z> {};
extern ZAxisStepper zAxisStepper;

class EAxisStepperConfig {
 public:
  class EnableRegister WREGISTER_BOOL(L, 0, true);       /*!< Pin 49. */
  class DirectionRegister WREGISTER_BOOL(L, 4, false);   /*!< Pin 39. */
  class PulseRegister WREGISTER_BOOL(G, 0, false);       /*!< Pin 41. */
  class ResolutionRegister0 WREGISTER_BOOL(L, 2, false); /*!< Pin 47. */
  class ResolutionRegister1 WREGISTER_BOOL(L, 4, false); /*!< Pin 45. */
  class ResolutionRegister2 WREGISTER_BOOL(L, 6, false); /*!< Pin 43. */
};
class EAxisStepper
    : public Clef::If::StepperPartial<EAxisStepperConfig, USTEPS_PER_MM_E> {};
extern EAxisStepper eAxisStepper;
}  // namespace Clef::Impl::Atmega2560
