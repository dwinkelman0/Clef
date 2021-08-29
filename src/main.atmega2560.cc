// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include <fw/GcodeParser.h>
#include <impl/atmega2560/Clock.h>
#include <impl/atmega2560/Interrupts.h>
#include <impl/atmega2560/PwmTimer.h>
#include <impl/atmega2560/Register.h>
#include <impl/atmega2560/Serial.h>
#include <stdio.h>

Clef::Impl::Atmega2560::Clock clock(Clef::Impl::Atmega2560::clockTimer);
Clef::Impl::Atmega2560::Usart serial;
Clef::Fw::ActionQueue actionQueue;
Clef::Fw::XYEPositionQueue xyePositionQueue;
Clef::Fw::GcodeParser gcodeParser(serial, actionQueue, xyePositionQueue);

void printSomething(void *arg) {
  uint64_t micros = *clock.getMicros();
  char buffer[32];
  sprintf(buffer, "t = %lu", micros);
  {
    Clef::Impl::Atmega2560::EnableInterrupts interrupts;
    serial.writeLine(buffer);
  }
  Clef::Impl::Atmega2560::pin8.write(
      !Clef::Impl::Atmega2560::pin8.getCurrentState());
}

int main() {
  if (clock.init()) {
    serial.writeLine("Initialized clock");
  }
  Clef::Impl::Atmega2560::pin8.init();
  Clef::Impl::Atmega2560::xAxisTimer.init();
  Clef::Impl::Atmega2560::xAxisTimer.setRisingEdgeCallback(printSomething,
                                                           nullptr);
  Clef::Impl::Atmega2560::xAxisTimer.setFallingEdgeCallback(printSomething,
                                                            nullptr);
  Clef::Impl::Atmega2560::xAxisTimer.setFrequency(10.0f);
  Clef::Impl::Atmega2560::xAxisTimer.enable();

  gcodeParser.init();
  while (1) {
    gcodeParser.ingest();
  }
}
