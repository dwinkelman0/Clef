// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <if/PwmTimer.h>
#include <if/Register.h>
#include <stdint.h>
#include <util/Units.h>

namespace Clef::If {
/**
 * Abstraction of an axis, cartesian or otherwise.
 */
template <uint32_t USTEPS_PER_MM>
class Axis {
 private:
  using PositionUnit = Clef::Util::PositionUnit;
  using TimeUnit = Clef::Util::TimeUnit;

 public:
  static const uint32_t UstepsPerMm = USTEPS_PER_MM;
  template <typename DType, PositionUnit PositionU>
  using Position = Clef::Util::Position<DType, PositionU, USTEPS_PER_MM>;
  template <typename DType, PositionUnit PositionU, TimeUnit TimeU>
  using Feedrate = Clef::Util::Feedrate<DType, PositionU, TimeU, USTEPS_PER_MM>;

  /**
   * Abstract representation of the direction of the stepper motor
   * associated with this axis.
   */
  class StepperDirectionRegister : protected RWRegister<uint8_t> {
    virtual void setIncreasing() = 0;
    virtual void setDecreasing() = 0;
    virtual void getIsIncreasing() = 0;
  };

  /**
   * Abstract representation of the resolution of the stepper motor driver
   * associated with this axis.
   */
  class StepperResolutionRegister : protected RWRegister<uint8_t> {
    /**
     * Possible resolution values for the DRV8825 stepper motor driver; these
     * values are microsteps per step (_32 is the best resolution).
     */
    enum class Resolution { _1 = 0, _2 = 1, _4 = 2, _8 = 3, _16 = 4, _32 = 5 };
    virtual void setResolution(const Resolution resolution) = 0;
    virtual Position<int32_t, PositionUnit::USTEP> getMicrostepsPerPulse() = 0;
  };

  void init() {
    stepperDirectionRegister_.setIncreasing();
    stepperResolutionRegister_.setResolution(
        StepperResolutionRegister::Resolution::_1);
  }

  /**
   * Reserve the axis so that the underlying resources are available for an
   * operation (such as performing a movement).
   */
  virtual void acquire() = 0;

  /**
   * Release the reservation.
   */
  virtual void release() = 0;

  /**
   * Release all reservations.
   */
  virtual void releaseAll() = 0;

  virtual void setTargetPos(
      const Position<int32_t, PositionUnit::USTEP> pos) = 0;
  virtual Position<int32_t, PositionUnit::USTEP> getCurrentPos() const = 0;
  virtual void setFeedrate(
      const Feedrate<int32_t, PositionUnit::USTEP, TimeUnit::MIN> feedrate) = 0;

 private:
  StepperDirectionRegister&
      stepperDirectionRegister_; /*!< Direction state of the stepper motor */
  StepperResolutionRegister&
      stepperResolutionRegister_; /*!< Resolution state of the stepper motor
                                   driver */
};

class XAxis : public Axis<160> {};
class YAxis : public Axis<160> {};
class ZAxis : public Axis<400> {};
class EAxis : public Axis<1280> {};
}  // namespace Clef::If
