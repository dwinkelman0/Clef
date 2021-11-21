// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <impl/atmega2560/Register.h>
#include <stdint.h>
#include <util/Initialized.h>

namespace Clef::Impl::Atmega2560 {
class AnalogBank : public Clef::Util::Initialized {
 public:
  using RawConversionCallback = void (*)(const uint16_t, void *);
  using ScaledConversionCallback = void (*)(const float, void *);

  AnalogBank();
  bool init() override;
  void addInput(const uint8_t number, RawConversionCallback callback,
                void *data);
  void addInput(const uint8_t number, ScaledConversionCallback callback,
                void *data);
  void removeInput(const uint8_t number);
  void handleConversion();
  static void onPwmTimerEdge(void *arg);

 private:
  void addInputCommon(const uint8_t number, void *data);
  static void initiateConversion(uint8_t number);

  union CallbackVariant {
    RawConversionCallback raw;
    ScaledConversionCallback scaled;
  };

  uint8_t currentInput_;
  uint16_t activeInputs_;
  uint16_t inputModes_; /*!< Bit is 0 if raw conversion, 1 if scaled. */
  uint8_t numActiveInputs_;
  CallbackVariant conversionCallbacks_[16];
  void *conversionCallbackData_[16];
};

extern AnalogBank analogBank;
}  // namespace Clef::Impl::Atmega2560
