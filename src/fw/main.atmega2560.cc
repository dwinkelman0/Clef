// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include <impl/hw/atmega2560/Serial.h>
#include <stdio.h>

Clef::Impl::Hw::Atmega2560::Usart serial;

int main() {
  serial.init();
  uint32_t counter = 0;
  while (1) {
    char c;
    if (serial.read(&c)) {
      char buffer[16];
      sprintf(buffer, "Received: %d", (int)c);
      serial.writeLine(buffer);
    }
  }
}
