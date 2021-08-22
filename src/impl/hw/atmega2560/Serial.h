// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <stdint.h>

#include "../../../if/hw/Serial.h"

namespace Clef::Impl::Hw::Atmega2560 {
class Usart : public Clef::If::Hw::RWSerial {
 public:
  void init() override;
  bool readyToRead() const override;
  bool read(char *const c) override;
  void writeChar(const char c) override;
  void writeStr(const char *str) override;
  void writeLine(const char *line) override;
};
}  // namespace Clef::Impl::Hw::Atmega2560
