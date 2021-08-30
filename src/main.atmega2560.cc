// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include <fw/Action.h>
#include <fw/GcodeParser.h>
#include <impl/atmega2560/Clock.h>
#include <impl/atmega2560/Interrupts.h>
#include <impl/atmega2560/PwmTimer.h>
#include <impl/atmega2560/Register.h>
#include <impl/atmega2560/Serial.h>
#include <impl/atmega2560/Stepper.h>
#include <stdio.h>

Clef::Impl::Atmega2560::Clock clock(Clef::Impl::Atmega2560::clockTimer);
Clef::Impl::Atmega2560::Usart serial;
Clef::Fw::ActionQueue actionQueue;
Clef::Fw::XYEPositionQueue xyePositionQueue;
Clef::Fw::GcodeParser gcodeParser(serial, actionQueue, xyePositionQueue);
Clef::Fw::Axes::XAxis xAxis(Clef::Impl::Atmega2560::xAxisStepper,
                            Clef::Impl::Atmega2560::xAxisTimer);

void status(void *arg) {
  char buffer[64];
  sprintf(buffer, "Position = %ld", *xAxis.getPosition());
  {
    Clef::Impl::Atmega2560::DisableInterrupts noInterrupts();
    serial.writeLine(buffer);
  }
}

int main() {
  if (clock.init()) {
    serial.writeLine("Initialized clock");
  }
  gcodeParser.init();
  xAxis.init();
  xAxis.setFeedrate(100000.0f);
  xAxis.setTargetPosition(1000000.0f);

  Clef::Impl::Atmega2560::yAxisTimer.init();
  Clef::Impl::Atmega2560::yAxisTimer.setFrequency(1.0f);
  Clef::Impl::Atmega2560::yAxisTimer.setRisingEdgeCallback(status, nullptr);
  Clef::Impl::Atmega2560::yAxisTimer.enable();

  while (1) {
    gcodeParser.ingest();
  }
}
