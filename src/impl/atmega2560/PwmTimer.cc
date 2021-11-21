// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "PwmTimer.h"

#include <avr/interrupt.h>
#include <if/Interrupts.h>
#include <math.h>

namespace Clef::Impl::Atmega2560 {
template <typename DType>
GenericTimer<DType>::GenericTimer() : dutyCycle_(0.5f) {}

template <typename DType>
bool GenericTimer<DType>::init() {
  if (Clef::Util::Initialized::init()) {
    Clef::If::DisableInterrupts noInterrupts();
    this->initRegs();
    return true;
  }
  return false;
}

template <typename DType>
void GenericTimer<DType>::enable() {
  this->setEnabled(true);
}

template <typename DType>
void GenericTimer<DType>::disable() {
  this->setEnabled(false);
}

template <typename DType>
void GenericTimer<DType>::setFrequency(
    const Clef::Util::Frequency<float> frequency) {
  using Prescaling = typename HardwareTimer<DType>::Prescaling;
  const Clef::Util::Frequency<float> clockFrequency(F_CPU);
  uint16_t divisor = 1;
  Prescaling prescaling;
  float baseFrequency = static_cast<float>(this->getMaxValue()) + 1;
  if (frequency > (clockFrequency / baseFrequency)) {
    prescaling = Prescaling::_1;
    divisor = 1;
  } else if (frequency > (clockFrequency / baseFrequency / 8)) {
    prescaling = Prescaling::_8;
    divisor = 8;
  } else if (frequency > (clockFrequency / baseFrequency / 64)) {
    prescaling = Prescaling::_64;
    divisor = 64;
  } else if (frequency > (clockFrequency / baseFrequency / 256)) {
    prescaling = Prescaling::_256;
    divisor = 256;
  } else if (frequency > (clockFrequency / baseFrequency / 1024)) {
    prescaling = Prescaling::_1024;
    divisor = 1024;
  } else {
    prescaling = Prescaling::_1024;
    divisor = 0;
  }
  DType compare =
      divisor == 0
          ? this->getMaxValue()
          : static_cast<DType>(clockFrequency / divisor / frequency - 1);
  {
    Clef::If::DisableInterrupts noInterrupts;
    this->setCompareA(compare);
    this->setCompareB(static_cast<DType>(compare * dutyCycle_));
    this->setPrescaler(prescaling);
    if (this->getCount() >= compare) {
      this->setCount(0);
    }
  }
}

template <typename DType>
Clef::Util::Frequency<float> GenericTimer<DType>::getMinFrequency() const {
  return static_cast<const HardwareTimer<DType> *>(this)->_getMinFrequency();
}

template <typename DType>
void GenericTimer<DType>::setDutyCycle(const float dutyCycle) {
  dutyCycle_ = 1 - dutyCycle;
  this->setCompareB(static_cast<DType>(dutyCycle_ * this->getCompareA()));
}

template <typename DType>
void GenericTimer<DType>::setRisingEdgeCallback(
    const TransitionCallback callback, void *data) {
  Clef::If::DisableInterrupts noInterrupts();
  risingEdgeCallback_ = callback;
  risingEdgeCallbackData_ = data;
}

template <typename DType>
void GenericTimer<DType>::setFallingEdgeCallback(
    const TransitionCallback callback, void *data) {
  Clef::If::DisableInterrupts noInterrupts();
  fallingEdgeCallback_ = callback;
  fallingEdgeCallbackData_ = data;
}

template <typename DType>
bool GenericDirectOutputTimer<DType>::init() {
  if (Clef::Util::Initialized::init()) {
    Clef::If::DisableInterrupts noInterrupts();
    this->initRegs();
    this->setPrescaler(HardwareTimer<DType>::Prescaling::_64);
    return true;
  }
  return false;
}

template <typename DType>
void GenericDirectOutputTimer<DType>::enable() {
  this->setEnabled(true);
}

template <typename DType>
void GenericDirectOutputTimer<DType>::disable() {
  this->setEnabled(false);
}

template <>
void GenericDirectOutputTimer<uint8_t>::setDutyCycleA(const float dutyCycle) {
  this->setCompareA(static_cast<uint8_t>(dutyCycle * 0xff));
}

template <>
void GenericDirectOutputTimer<uint8_t>::setDutyCycleB(const float dutyCycle) {
  this->setCompareB(static_cast<uint8_t>(dutyCycle * 0xff));
}

template <>
float GenericDirectOutputTimer<uint8_t>::getDutyCycleA() const {
  return this->getCompareA() / static_cast<float>(0xff);
}

template <>
float GenericDirectOutputTimer<uint8_t>::getDutyCycleB() const {
  return this->getCompareB() / static_cast<float>(0xff);
}

template <>
void GenericDirectOutputTimer<uint8_t>::setCallbackA(
    const TransitionCallback callback, void *data) {
  callbackA_ = callback;
  callbackAData_ = data;
}

template <>
void GenericDirectOutputTimer<uint8_t>::setCallbackB(
    const TransitionCallback callback, void *data) {
  callbackB_ = callback;
  callbackBData_ = data;
}

template <>
void GenericDirectOutputTimer<uint8_t>::setCallbackTop(
    const TransitionCallback callback, void *data) {
  callbackTop_ = callback;
  callbackTopData_ = data;
}

/**
 * Create ISRs for each timer.
 */
#define TIMER_ISRS(NAME, N)                                     \
  ISR(REG3(TIMER, N, _COMPA_vect)) {                            \
    if (NAME.fallingEdgeCallback_) {                            \
      NAME.fallingEdgeCallback_(NAME.fallingEdgeCallbackData_); \
    }                                                           \
  }                                                             \
  ISR(REG3(TIMER, N, _COMPB_vect)) {                            \
    if (NAME.risingEdgeCallback_) {                             \
      NAME.risingEdgeCallback_(NAME.risingEdgeCallbackData_);   \
    }                                                           \
  }

#define TIMER_PWM_ISRS(NAME, N)                 \
  ISR(REG3(TIMER, N, _COMPA_vect)) {            \
    if (NAME.callbackA_) {                      \
      NAME.callbackA_(NAME.callbackAData_);     \
    }                                           \
  }                                             \
  ISR(REG3(TIMER, N, _COMPB_vect)) {            \
    if (NAME.callbackB_) {                      \
      NAME.callbackB_(NAME.callbackBData_);     \
    }                                           \
  }                                             \
  ISR(REG3(TIMER, N, _OVF_vect)) {              \
    if (NAME.callbackTop_) {                    \
      NAME.callbackTop_(NAME.callbackTopData_); \
    }                                           \
  }

Timer1 timer1;
Timer2 timer2;
ClockTimer clockTimer;
XAxisTimer xAxisTimer;
YAxisTimer yAxisTimer;
ZEAxisTimer zeAxisTimer;

TIMER_ISRS(timer1, 0);
TIMER_PWM_ISRS(timer2, 2);
TIMER_ISRS(clockTimer, 1);
TIMER_ISRS(xAxisTimer, 3);
TIMER_ISRS(yAxisTimer, 4);
TIMER_ISRS(zeAxisTimer, 5);
}  // namespace Clef::Impl::Atmega2560
