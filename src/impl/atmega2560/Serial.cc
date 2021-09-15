// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "Serial.h"

#include <if/Interrupts.h>
#include <impl/atmega2560/Config.h>
#include <impl/atmega2560/Register.h>
#include <util/delay.h>

extern "C" {
#define USE_USART0
#include "usart/usart.h"
}

namespace Clef::Impl::Atmega2560 {
bool Usart::init() {
  if (Clef::Util::Initialized::init()) {
    uart_init(BAUD_CALC(SERIAL_BAUDRATE));
    return true;
  }
  return false;
}

bool Usart::isReadyToRead() const { return uart_AvailableBytes() > 0; }

bool Usart::read(char *const c) {
  if (isReadyToRead()) {
    *c = uart_getc();
    return true;
  } else {
    *c = '\0';
    return false;
  }
}

void Usart::writeChar(const char c) { uart_putc(c); }

void Usart::writeStr(const char *str) { uart_putstr(const_cast<char *>(str)); }

void Usart::writeLine(const char *line) {
  writeStr(line);
  writeChar('\n');
}

Usart serial;

namespace {
class WPinSS W_REGISTER_BOOL(B, 0, true);    /*!< Pin 53. */
class WPinSCK W_REGISTER_BOOL(B, 1, false);  /*!< Pin 52. */
class WPinMOSI W_REGISTER_BOOL(B, 2, false); /*!< Pin 51. */
class WPinMISO R_REGISTER_BOOL(B, 3);        /*!< Pin 50. */
}  // namespace

Spi::Spi()
    : Clef::If::RSpi::RSpi(), isReady_(true), readSize_(0), numBytesRead_(0) {}

bool Spi::init() {
  WPinSS::init();
  WPinSS::write(false);
  WPinSCK::init();
  WPinMOSI::init();
  WPinMISO::init();
  WPinMISO::setPullUp();
  SPCR = (1 << SPE) |
         (1 << MSTR);  /*!< Enable SPI in master mode; implicitly set CPOL as 0,
                          CPHA as 0, and data order as MSB-first. */
  SPCR |= (1 << SPR1); /*!< Set serial clock to 250 kHz. */
  SPSR |= (1 << SPI2X); /*!< Double serial clock frequency (to  500 kHz). */
  SPCR |= (1 << SPIE);  /*!< Enable SPI interrupts. */
  return true;
}

bool Spi::initRead(
    const uint16_t size,
    const Clef::Util::Time<uint16_t, Clef::Util::TimeUnit::USEC> delay) {
  {
    Clef::If::DisableInterrupts noInterrupts;
    if (!isReady_ || size > maxSize_) {
      return false;
    }
    isReady_ = false;
  }
  readSize_ = size;
  numBytesRead_ = 0;
  WPinSS::write(true);
  _delay_loop_2(*delay * 4); /*!< Each unit of delay is 4 clock cycles. */
  SPDR = 0xff;
  return true;
}

void Spi::onByteRead() {
  buffer_[numBytesRead_++] = SPDR;
  if (numBytesRead_ < readSize_) {
    SPDR = 0xff;
  } else {
    {
      Clef::If::EnableInterrupts interrupts;
      WPinSS::write(false);
      if (readCompleteCallback_) {
        readCompleteCallback_(readSize_, buffer_, readCompleteCallbackData_);
      }
    }
    isReady_ = true;
  }
}

ISR(SPI_STC_vect) { spi.onByteRead(); }

Spi spi;
}  // namespace Clef::Impl::Atmega2560
