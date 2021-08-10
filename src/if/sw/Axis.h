// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <stdint.h>

#include "../hw/Register.h"

namespace Clef::If::Sw {
/**
 * Abstraction of an axis, cartesian or otherwise.
 */
template <uint32_t USTEPS_PER_MM>
class Axis {
 public:
  enum class PositionUnit { USTEPS, MMS };
  enum class TimeUnit { MIN, SEC, MSEC, USEC };

  /**
   * Offer a variety of parametrizations for position of the axis.
   */
  template <typename DType, PositionUnit PositionU>
  class Position {
   public:
    constexpr Position(DType position) : position_(position) {}
    inline constexpr DType operator*() const { return position_; }

   private:
    DType position_;
  };

  /**
   * Offer a variety of parametrizations for feedrate of the axis.
   */
  template <typename DType, PositionUnit PositionU, TimeUnit TimeU>
  class Feedrate {
   public:
    constexpr Feedrate(DType feedrate) : feedrate_(feedrate) {}
    inline constexpr DType operator*() const { return feedrate_; }

   private:
    DType feedrate_;
  };

  /**
   * Abstract representation of the direction of the stepper motor associated
   * with this axis.
   */
  class DirectionRegister : protected RWRegister<uint8_t> {
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
     * values are microsteps per pulse.
     */
    enum class Resolution { _1 = 0, _2 = 1, _4 = 2, _8 = 3, _16 = 4, _32 = 5 };
    inline void setResolution(const Resolution resolution) {
      write(reinterpret_cast<uint8_t>(resolution));
    }
    inline Position<uint8_t, PositionUnit::USTEPS> getMicrostepsPerPulse()
        const {
      return 1 << read();
    }
  };

  void init() { directionRegister_.setIncreasing(); }

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

  virtual void setTargetPos(const Position<int32_t, Unit::USTEPS> pos) = 0;
  virtual Position<int32_t, Unit::USTEPS> getCurrentPos() const = 0;
  virtual void setFeedrate(
      const Feedrate<int32_t, Unit::USTEPS, Unit::MIN> feedrate) = 0;

 private:
  DirectionRegister
      directionRegister_; /*!< Direction state of the stepper motor */
};
}  // namespace Clef::If::Sw
