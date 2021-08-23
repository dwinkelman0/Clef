// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <if/Serial.h>
#include <stdint.h>

namespace Clef::Impl::Atmega2560 {
class Usart : public Clef::If::RWSerial {
 public:
  void init() override;
  bool isReadyToRead() const override;
  bool read(char *const c) override;
  void writeChar(const char c) override;
  void writeStr(const char *str) override;
  void writeLine(const char *line) override;

 private:
  /**
   * Prevent multiple initializations.
   */
  bool isInitialized_ = false;
};
}  // namespace Clef::Impl::Atmega2560
