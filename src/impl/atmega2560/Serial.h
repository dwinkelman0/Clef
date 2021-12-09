// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <if/Serial.h>
#include <impl/atmega2560/AvrUtils.h>
#include <impl/atmega2560/Config.h>
#include <stdint.h>

extern "C" {
#define USE_USART0
#include "usart/usart.h"
}

namespace Clef::Impl::Atmega2560 {
class UsartPartial : public Clef::If::RWSerial {
 public:
  void writeUint64(const uint64_t x) override {
    char buffer[24];
    uint8_t numChars = 0;
    uint64_t remainder = x;
    if (remainder == 0) {
      buffer[0] = '0';
      numChars = 1;
    } else {
      while (remainder > 0) {
        uint8_t digit = remainder % 10;
        buffer[numChars++] = '0' + digit;
        remainder /= 10;
      }
    }
    char buffer2[24];
    for (int i = numChars - 1, j = 0; j < numChars; --i, ++j) {
      buffer2[j] = buffer[i];
    }
    buffer2[numChars] = '\0';
    writeStr(buffer2);
  }
};

#define USART(N, BAUDRATE)                                              \
 public                                                                 \
  UsartPartial {                                                        \
   public:                                                              \
    bool init() override {                                              \
      if (Clef::Util::Initialized::init()) {                            \
        REG3(uart, N, _init)(BAUD_CALC(BAUDRATE));                      \
        return true;                                                    \
      }                                                                 \
      return false;                                                     \
    }                                                                   \
    bool isReadyToRead() const override {                               \
      return REG3(uart, N, _AvailableBytes)() > 0;                      \
    };                                                                  \
    bool read(char *const c) override {                                 \
      if (isReadyToRead()) {                                            \
        *c = REG3(uart, N, _getc)();                                    \
        return true;                                                    \
      } else {                                                          \
        *c = '\0';                                                      \
        return false;                                                   \
      }                                                                 \
    };                                                                  \
    void writeChar(const char c) override { REG3(uart, N, _putc)(c); }; \
    void writeStr(const char *str) override {                           \
      REG3(uart, N, _putstr)(const_cast<char *>(str));                  \
    };                                                                  \
    void writeLine(const char *line) override {                         \
      writeStr(line);                                                   \
      writeChar('\n');                                                  \
    }                                                                   \
  }

class Usart0 : USART(0, SERIAL_BAUDRATE);
extern Usart0 serial;

class Usart1 : USART(1, SERIAL_BAUDRATE);
extern Usart1 serial1;

class Usart2 : USART(2, MASS_SENSOR_SERIAL_BAUDRATE);
extern Usart2 serial2;

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
