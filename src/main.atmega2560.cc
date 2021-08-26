// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include <fw/GcodeParser.h>
#include <impl/atmega2560/Serial.h>
#include <stdio.h>

int main() {
  Clef::Impl::Atmega2560::Usart serial;
  Clef::Fw::ActionQueue actionQueue;
  Clef::Fw::XYEPositionQueue xyePositionQueue;
  Clef::Fw::GcodeParser gcodeParser(serial, actionQueue, xyePositionQueue);
  gcodeParser.init();
  while (1) {
    gcodeParser.ingest();
  }
}
