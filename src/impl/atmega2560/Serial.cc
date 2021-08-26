// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include "Serial.h"

#include "config.h"

extern "C" {
#define USE_USART0
#include <avr/interrupt.h>

#include "usart/usart.h"
}

namespace Clef::Impl::Atmega2560 {
void Usart::init() {
  if (!isInitialized_) {
    uart_init(BAUD_CALC(SERIAL_BAUDRATE));
    sei();
    isInitialized_ = true;
  }
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
}  // namespace Clef::Impl::Atmega2560