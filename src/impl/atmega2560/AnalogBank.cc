// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "AnalogBank.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <if/Interrupts.h>
#include <string.h>

namespace Clef::Impl::Atmega2560 {
AnalogBank::AnalogBank()
    : currentInput_(0), activeInputs_(0), numActiveInputs_(0) {
  memset(conversionCallbacks_, 0, sizeof(conversionCallbacks_));
  memset(conversionCallbackData_, 0, sizeof(conversionCallbackData_));
}

bool AnalogBank::init() {
  Pin6::init();
  ADMUX = 0; /*!< Internal voltage reference, right-adjusted output bits. */
  ADCSRB &= ~(1 << MUX5); /*!< Reset multiplexer. */
  ADCSRA = (1 << ADEN) | (1 << ADIE) | (1 << ADPS2) | (1 << ADPS1) |
           (1 << ADPS0); /*!< Enable ADC, enable conversion interrupt, disable
                            auto-triggering, maximum prescaling. */
  DIDR0 = 0;             /*!< Allow digital inputs to be enabled. */
  DIDR2 = 0;
  return true;
}

void AnalogBank::addInput(const uint8_t number, ConversionCallback callback,
                          void *data) {
  Clef::If::DisableInterrupts disableInterrupts;
  activeInputs_ |= 1 << number;
  numActiveInputs_++;
  conversionCallbacks_[number] = callback;
  conversionCallbackData_[number] = data;
}

void AnalogBank::removeInput(const uint8_t number) {
  Clef::If::DisableInterrupts disableInterrupts;
  activeInputs_ &= ~(1 << number);
  numActiveInputs_--;
  conversionCallbacks_[number] = nullptr;
  conversionCallbackData_[number] = nullptr;
}

void AnalogBank::handleConversion() {
  uint8_t lsbs = ADCL;
  uint8_t msbs = ADCH;
  if (conversionCallbacks_[currentInput_]) {
    conversionCallbacks_[currentInput_](
        (static_cast<uint16_t>(msbs) << 8) | static_cast<uint16_t>(lsbs),
        conversionCallbackData_[currentInput_]);
  }
}

void AnalogBank::onPwmTimerEdge(void *arg) {
  AnalogBank *analogBank = reinterpret_cast<AnalogBank *>(arg);
  if (analogBank->numActiveInputs_ == 0) {
    // Do nothing
  } else if (analogBank->numActiveInputs_ == 1) {
    initiateConversion(analogBank->currentInput_);
  } else {
    for (uint8_t nextInput = (analogBank->currentInput_ + 1) & 0x0f;
         nextInput != analogBank->currentInput_;
         nextInput = (nextInput + 1) & 0x0f) {
      if ((analogBank->activeInputs_ >> nextInput) & 0x01) {
        analogBank->currentInput_ = nextInput;
        initiateConversion(nextInput);
        break;
      }
    }
  }
}

void AnalogBank::initiateConversion(uint8_t number) {
  Pin6::write(!Pin6::getCurrentState());
  ADMUX &= ~0x07;
  ADMUX |= number & 0x07;
  ADCSRB &= ~(1 << MUX5);
  ADCSRB |= (number >> 3) << MUX5;
  ADCSRA |= 1 << ADSC; /*!< Start conversion. */
}

ISR(ADC_vect) {
  Pin6::write(!Pin6::getCurrentState());
  analogBank.handleConversion();
}

AnalogBank analogBank;
}  // namespace Clef::Impl::Atmega2560
