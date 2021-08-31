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
Clef::Fw::Axes::YAxis yAxis(Clef::Impl::Atmega2560::yAxisStepper,
                            Clef::Impl::Atmega2560::yAxisTimer);
Clef::Fw::Axes::ZAxis zAxis(Clef::Impl::Atmega2560::zAxisStepper,
                            Clef::Impl::Atmega2560::zeAxisTimer);
Clef::Fw::Axes::EAxis eAxis(Clef::Impl::Atmega2560::eAxisStepper,
                            Clef::Impl::Atmega2560::zeAxisTimer);
Clef::Fw::Axes axes(xAxis, yAxis, zAxis, eAxis);

void status(void *arg) {
  Clef::Impl::Atmega2560::EnableInterrupts interrupts();
  char buffer[64];
  sprintf(buffer, "Position = (%ld, %ld, %ld, %ld)", *axes.getX().getPosition(),
          *axes.getY().getPosition(), *axes.getZ().getPosition(),
          *axes.getE().getPosition());
  serial.writeLine(buffer);
}

int main() {
  if (clock.init()) {
    serial.writeLine("Initialized clock");
  }
  gcodeParser.init();
  axes.init();
  axes.getY().setFeedrate(100000.0f);
  axes.getY().setTargetPosition(1000000.0f);
  axes.getX().setFeedrate(60000.0f);
  axes.getX().setTargetPosition(600000.f);

  Clef::Impl::Atmega2560::zeAxisTimer.init();
  Clef::Impl::Atmega2560::zeAxisTimer.setFrequency(4.0f);
  Clef::Impl::Atmega2560::zeAxisTimer.setRisingEdgeCallback(status, nullptr);
  Clef::Impl::Atmega2560::zeAxisTimer.enable();

  while (1) {
    gcodeParser.ingest();
  }
}
