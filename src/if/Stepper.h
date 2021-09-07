// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <if/Interrupts.h>
#include <stdint.h>
#include <util/Acquired.h>
#include <util/Initialized.h>
#include <util/Units.h>

namespace Clef::If {
template <uint32_t USTEPS_PER_MM>
class Stepper : public Clef::Util::Initialized, public Clef::Util::Acquired {
 public:
  using Position =
      Clef::Util::Position<int32_t, Clef::Util::PositionUnit::USTEP,
                           USTEPS_PER_MM>;

  /**
   * Resolution measured as microsteps per step (i.e. _32 is best).
   */
  enum class Resolution : uint8_t {
    _1 = 0,
    _2 = 1,
    _4 = 2,
    _8 = 3,
    _16 = 4,
    _32 = 5
  };

  virtual void setResolution(const Resolution resolution) = 0;
  virtual Resolution getResolution() const = 0;
  virtual void pulse() = 0;
  virtual void unpulse() = 0;
  virtual void setTargetPosition(const Position position) = 0;
  virtual Position getTargetPosition() const = 0;
  virtual Position getPosition() = 0;
  virtual bool isAtTargetPosition() const = 0;
};

template <typename Config, uint32_t USTEPS_PER_MM>
class StepperPartial : public Stepper<USTEPS_PER_MM> {
 public:
  using Position = typename Stepper<USTEPS_PER_MM>::Position;
  using Resolution = typename Stepper<USTEPS_PER_MM>::Resolution;

  StepperPartial() : ustepsPerPulse_(1), position_(0), targetPosition_(0) {}

  bool init() override {
    Clef::If::DisableInterrupts noInterrupts;
    Config::EnableRegister::init();
    Config::DirectionRegister::init();
    Config::PulseRegister::init();
    Config::ResolutionRegister0::init();
    Config::ResolutionRegister1::init();
    Config::ResolutionRegister2::init();
    Config::EnableRegister::write(false);
    setIncreasing();
    setResolution(Resolution::_32);
    this->releaseAll();
    return true;
  }

  void onFirstAcquire() override { Config::EnableRegister::write(true); }

  void onLastRelease() override { Config::EnableRegister::write(false); }

  void setIncreasing() {
    Config::DirectionRegister::write(true);
    updateUstepsPerPulse();
  }

  void setDecreasing() {
    Config::DirectionRegister::write(false);
    updateUstepsPerPulse();
  }

  bool isIncreasing() const {
    return Config::DirectionRegister::getCurrentState();
  }

  void setResolution(const Resolution resolution) override {
    uint8_t intEncoding = static_cast<uint8_t>(resolution);
    Config::ResolutionRegister0::write(intEncoding & 1);
    Config::ResolutionRegister1::write((intEncoding >> 1) & 1);
    Config::ResolutionRegister2::write((intEncoding >> 2) & 1);
    updateUstepsPerPulse();
  }

  Resolution getResolution() const override {
    return static_cast<Resolution>(
        static_cast<uint8_t>(Config::ResolutionRegister0::getCurrentState()) |
        (static_cast<uint8_t>(Config::ResolutionRegister1::getCurrentState())
         << 1) |
        (static_cast<uint8_t>(Config::ResolutionRegister2::getCurrentState())
         << 2));
  }

  Position getUstepsPerPulse() { return ustepsPerPulse_; }

  void pulse() override {
    Config::PulseRegister::write(true);
    position_ = position_ + getUstepsPerPulse();
  }

  void unpulse() override { Config::PulseRegister::write(false); }

  void setTargetPosition(const Position position) override {
    if (position > targetPosition_) {
      setIncreasing();
    } else {
      setDecreasing();
    }
    targetPosition_ = position;
  }

  Position getTargetPosition() const override { return targetPosition_; }

  Position getPosition() override { return position_; }

  bool isAtTargetPosition() const override {
    return isIncreasing() ? position_ >= this->getTargetPosition()
                          : position_ <= this->getTargetPosition();
  }

 private:
  void updateUstepsPerPulse() {
    ustepsPerPulse_ = (1 << (5 - static_cast<uint8_t>(getResolution()))) *
                      (isIncreasing() ? 1 : -1);
  }

  Position ustepsPerPulse_;
  Position position_;
  Position targetPosition_;
};
}  // namespace Clef::If
