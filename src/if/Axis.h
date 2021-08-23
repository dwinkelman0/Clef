// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <if/PwmTimer.h>
#include <if/Register.h>
#include <stdint.h>

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
  template <typename DType, PositionUnit PositionU>
  using Position = Clef::Util::Position<DType, PositionU, USTEPS_PER_MM>;
  template <typename DType, PositionUnit PositionU, TimeUnit TimeU>
  using Feedrate = Clef::Util::Feedrate<DType, PositionU, TimeU, USTEPS_PER_MM>;

  /**
   * Abstract representation of the direction of the stepper motor
   * associated with this axis.
   */
  class StepperDirectionRegister : protected RWRegister<uint8_t> {
    inline void setIncreasing() { write(1); }
    inline void setDecreasing() { write(0); }
    inline void getIsIncreasing() { return read(); }
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
    inline void setResolution(const Resolution resolution) {
      write(reinterpret_cast<uint8_t>(resolution));
    }
    inline Position<int32_t, PositionUnit::USTEP> getMicrostepsPerPulse()
        const {
      return 1 << read();
    }
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
      const Position<int32_t, PositionUnit::USTEPS> pos) = 0;
  virtual Position<int32_t, PositionUnit::USTEPS> getCurrentPos() const = 0;
  virtual void setFeedrate(
      const Feedrate<int32_t, PositionUnit::USTEPS, TimeUnit::MIN>
          feedrate) = 0;

 private:
  StepperDirectionRegister
      stepperDirectionRegister_; /*!< Direction state of the stepper motor */
  StepperResolutionRegister
      stepperResolutionRegister_; /*!< Resolution state of the stepper motor
                                     driver */
};
}  // namespace Clef::If
