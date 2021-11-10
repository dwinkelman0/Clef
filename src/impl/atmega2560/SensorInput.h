// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <if/Interrupts.h>
#include <if/PwmTimer.h>
#include <if/SensorInput.h>
#include <impl/atmega2560/Config.h>
#include <impl/atmega2560/PwmTimer.h>
#include <impl/atmega2560/Register.h>
#include <util/Units.h>

namespace Clef::Impl::Atmega2560 {
template <typename Config>
class Caliper : public Clef::If::SensorInput<typename Config::Position> {
 public:
  /**
   * This device will use the rising edge callback of the PwmTimer and set the
   * frequency to 250 Hz.
   */
  Caliper(Clef::If::PwmTimer &pwmTimer) : pwmTimer_(pwmTimer) {}

  bool init() override {
    Clef::If::DisableInterrupts noInterrupts;
    Config::ClockRegister::init();
    Config::ClockRegister::setCallback(onCaliperClockEdge, this);
    Config::DataRegister::init();
    pwmTimer_.init();
    pwmTimer_.setFrequency(250.0f);
    pwmTimer_.setRisingEdgeCallback(onPwmTimerEdge, this);
    pwmTimer_.enable();
    return true;
  }

 private:
  static void onPwmTimerEdge(void *arg) {
    Caliper *caliper = reinterpret_cast<Caliper *>(arg);
    if (caliper->numRawDataBits_ == 24) {
      if (caliper->conversionCallback_) {
        int32_t output = (~caliper->rawData_) & 0x3fff;
        if (~caliper->rawData_ & (static_cast<uint32_t>(1) << 20)) {
          output = -output;
        }
        // Flip the sign of the measurement so that increasing displacement
        // matches increasing extrusion
        caliper->conversionCallback_(
            typename Config::Position(static_cast<float>(-output) / 100),
            caliper->conversionCallbackData_);
      }
    }
    caliper->rawData_ = 0;
    caliper->numRawDataBits_ = 0;
  }

  static void onCaliperClockEdge(void *arg) {
    Caliper *caliper = reinterpret_cast<Caliper *>(arg);
    bool newBit = Config::DataRegister::read();
    caliper->rawData_ =
        (caliper->rawData_ | (static_cast<uint32_t>(newBit) << 24)) >> 1;
    caliper->numRawDataBits_++;

    // PwmTimer::enable() resets the counter
    caliper->pwmTimer_.enable();
  }

  Clef::If::PwmTimer &pwmTimer_;
  uint32_t rawData_;
  uint8_t numRawDataBits_;
};

class ExtruderCaliperConfig {
 public:
  using Position = Clef::Util::Position<float, Clef::Util::PositionUnit::MM,
                                        USTEPS_PER_MM_DISPLACEMENT>;
  class ClockRegister RINT_REGISTER_BOOL(
      E, 4, B, 4, RINT_REGISTER_BOOL_EDGE_FALLING); /*!< Pin 2. */
  class DataRegister R_REGISTER_BOOL(H, 5);         /*!< Pin 8. */
};
class ExtruderCaliper : public Caliper<ExtruderCaliperConfig> {
 public:
  ExtruderCaliper() : Caliper(Clef::Impl::Atmega2560::timer2) {}
};
extern ExtruderCaliper extruderCaliper;
}  // namespace Clef::Impl::Atmega2560
