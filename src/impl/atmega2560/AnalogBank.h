// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <impl/atmega2560/Register.h>
#include <stdint.h>
#include <util/Initialized.h>

namespace Clef::Impl::Atmega2560 {
class AnalogBank : public Clef::Util::Initialized {
 public:
  using ConversionCallback = void (*)(const uint16_t, void *);

  AnalogBank();
  bool init() override;
  void addInput(const uint8_t number, ConversionCallback callback, void *data);
  void removeInput(const uint8_t number);
  void handleConversion();
  static void onPwmTimerEdge(void *arg);

 private:
  static void initiateConversion(uint8_t number);

  uint8_t currentInput_;
  uint16_t activeInputs_;
  uint8_t numActiveInputs_;
  ConversionCallback conversionCallbacks_[16];
  void *conversionCallbackData_[16];
};

class Pin6 W_REGISTER_BOOL(H, 3, false);

extern AnalogBank analogBank;
}  // namespace Clef::Impl::Atmega2560
