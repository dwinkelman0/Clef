// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <if/Serial.h>
#include <stdint.h>

namespace Clef::Impl::Atmega2560 {
class Usart : public Clef::If::RWSerial {
 public:
  bool init() override;
  bool isReadyToRead() const override;
  bool read(char *const c) override;
  void writeChar(const char c) override;
  void writeStr(const char *str) override;
  void writeLine(const char *line) override;
};

extern Usart serial;

class Spi : public Clef::If::RSpi {
 public:
  Spi();
  bool init() override;
  bool initRead(const uint16_t size,
                const Clef::Util::Time<uint16_t, Clef::Util::TimeUnit::USEC>
                    delay) override;
  void onByteRead();

 private:
  bool isReady_;
  const uint16_t maxSize_ = 8;
  char buffer_[8];
  uint16_t readSize_;
  uint16_t numBytesRead_;
};

extern Spi spi;
}  // namespace Clef::Impl::Atmega2560
