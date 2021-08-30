// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <if/Stepper.h>
#include <impl/atmega2560/Config.h>
#include <impl/atmega2560/Register.h>

namespace Clef::Impl::Atmega2560 {
template <typename Config>
class Stepper : public Clef::If::Stepper<Config::USTEPS_PER_MM> {
 public:
  using Resolution =
      typename Clef::If::Stepper<Config::USTEPS_PER_MM>::Resolution;
  using Position = typename Clef::If::Stepper<Config::USTEPS_PER_MM>::Position;

  Stepper();
  bool init() override;
  void onFirstAcquire() override;
  void onLastRelease() override;
  void setIncreasing() override;
  void setDecreasing() override;
  bool isIncreasing() const override;
  void setResolution(const Resolution resolution) override;
  Resolution getResolution() const override;
  Position getUstepsPerPulse() const override;
  void pulse() override;
  void unpulse() override;
  Position getPosition() const override;
  bool isAtTargetPosition() const override;

 private:
  void updateUstepsPerPulse();

  Position ustepsPerPulse_;
  Position position_;
};

class XAxisStepperConfig {
 public:
  static const uint32_t USTEPS_PER_MM = USTEPS_PER_MM_X;
  class EnableRegister WREGISTER_BOOL(C, 1, true);       /*!< Pin 36. */
  class DirectionRegister WREGISTER_BOOL(A, 4, false);   /*!< Pin 26. */
  class PulseRegister WREGISTER_BOOL(A, 6, false);       /*!< Pin 28. */
  class ResolutionRegister0 WREGISTER_BOOL(C, 3, false); /*!< Pin 34. */
  class ResolutionRegister1 WREGISTER_BOOL(C, 5, false); /*!< Pin 32. */
  class ResolutionRegister2 WREGISTER_BOOL(C, 7, false); /*!< Pin 30. */
};
class XAxisStepper : public Stepper<XAxisStepperConfig> {};
extern XAxisStepper xAxisStepper;
}  // namespace Clef::Impl::Atmega2560
