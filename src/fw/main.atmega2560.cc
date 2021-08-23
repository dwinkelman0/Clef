// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include <if/GcodeParser.h>
#include <impl/atmega2560/Serial.h>
#include <stdio.h>

int main() {
  Clef::Impl::Atmega2560::Usart serial;
  Clef::If::GcodeParser gcodeParser(serial);
  gcodeParser.init();
  while (1) {
    gcodeParser.ingest();
  }
}
