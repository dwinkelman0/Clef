// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "Stepper.h"

namespace Clef::Impl::Atmega2560 {
template <typename Config>
Stepper<Config>::Stepper() : ustepsPerPulse_(0), position_(0) {}

template <typename Config>
bool Stepper<Config>::init() {
  Config::EnableRegister::init();
  Config::DirectionRegister::init();
  Config::PulseRegister::init();
  Config::ResolutionRegister0::init();
  Config::ResolutionRegister1::init();
  Config::ResolutionRegister2::init();
  Config::EnableRegister::write(false);
  setIncreasing();
  setResolution(Resolution::_32);
  ustepsPerPulse_ = Position(1);
  return true;
}

template <typename Config>
void Stepper<Config>::onFirstAcquire() {
  Config::EnableRegister::write(true);
}

template <typename Config>
void Stepper<Config>::onLastRelease() {
  Config::EnableRegister::write(false);
}

template <typename Config>
void Stepper<Config>::setIncreasing() {
  Config::DirectionRegister::write(true);
  updateUstepsPerPulse();
}

template <typename Config>
void Stepper<Config>::setDecreasing() {
  Config::DirectionRegister::write(false);
  updateUstepsPerPulse();
}

template <typename Config>
bool Stepper<Config>::isIncreasing() const {
  return Config::DirectionRegister::getCurrentState();
}

template <typename Config>
void Stepper<Config>::setResolution(const Resolution resolution) {
  uint8_t intEncoding = static_cast<uint8_t>(resolution);
  Config::ResolutionRegister0::write(intEncoding & 1);
  Config::ResolutionRegister1::write((intEncoding >> 1) & 1);
  Config::ResolutionRegister2::write((intEncoding >> 2) & 1);
  updateUstepsPerPulse();
}

template <typename Config>
typename Stepper<Config>::Resolution Stepper<Config>::getResolution() const {
  return static_cast<Resolution>(
      static_cast<uint8_t>(Config::ResolutionRegister0::getCurrentState()) |
      (static_cast<uint8_t>(Config::ResolutionRegister1::getCurrentState())
       << 1) |
      (static_cast<uint8_t>(Config::ResolutionRegister2::getCurrentState())
       << 2));
}

template <typename Config>
typename Stepper<Config>::Position Stepper<Config>::getUstepsPerPulse() const {
  return ustepsPerPulse_;
}

template <typename Config>
void Stepper<Config>::pulse() {
  Config::PulseRegister::write(true);
  position_ = position_ + getUstepsPerPulse();
}

template <typename Config>
void Stepper<Config>::unpulse() {
  Config::PulseRegister::write(false);
}

template <typename Config>
void Stepper<Config>::updateUstepsPerPulse() {
  ustepsPerPulse_ = (1 << (5 - static_cast<uint8_t>(getResolution()))) *
                    (isIncreasing() ? 1 : -1);
}

template <typename Config>
typename Stepper<Config>::Position Stepper<Config>::getPosition() const {
  return position_;
}

template <typename Config>
bool Stepper<Config>::isAtTargetPosition() const {
  return isIncreasing() ? position_ >= this->getTargetPosition()
                        : position_ <= this->getTargetPosition();
}

XAxisStepper xAxisStepper;
}  // namespace Clef::Impl::Atmega2560
