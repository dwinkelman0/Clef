// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include <avr/interrupt.h>
#include <fw/GcodeParser.h>
#include <impl/atmega2560/PwmTimer.h>
#include <impl/atmega2560/Serial.h>
#include <stdio.h>

Clef::Impl::Atmega2560::Usart serial;
Clef::Fw::ActionQueue actionQueue;
Clef::Fw::XYEPositionQueue xyePositionQueue;
Clef::Fw::GcodeParser gcodeParser(serial, actionQueue, xyePositionQueue);

void printSomething(void *arg) {
  sei();
  serial.writeLine("Hello");
  cli();
}

int main() {
  Clef::Impl::Atmega2560::xAxisTimer.init();
  Clef::Impl::Atmega2560::xAxisTimer.setRisingEdgeCallback(printSomething,
                                                           nullptr);
  Clef::Impl::Atmega2560::xAxisTimer.setFrequency(0.5f);
  Clef::Impl::Atmega2560::xAxisTimer.enable();

  gcodeParser.init();
  while (1) {
    gcodeParser.ingest();
  }
}
