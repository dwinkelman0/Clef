// Copyright 2021 by Daniel Winkelman. All rights reserved.

/**
 * This test case makes sure PWM timers of both varieties work. For this test,
 * expect that Arduino Mega pins 9 and 10 produce waveforms and all other pins
 * do not.
 */

#include <if/Interrupts.h>
#include <impl/atmega2560/Clock.h>
#include <impl/atmega2560/PwmTimer.h>
#include <impl/atmega2560/Serial.h>
#include <stdio.h>

static Clef::Impl::Atmega2560::Clock clock(Clef::Impl::Atmega2560::clockTimer);

static void printMessage(const char *message) {
  Clef::If::EnableInterrupts enableInterrupts;
  Clef::Impl::Atmega2560::serial.writeStr(message);
  Clef::Impl::Atmega2560::serial.writeStr(": ");
  Clef::Impl::Atmega2560::serial.writeUint64(*clock.getMicros());
  Clef::Impl::Atmega2560::serial.writeStr("\n");
}

static void onRisingEdgeX(void *arg) { printMessage("RisingX"); }

static void onFallingEdgeX(void *arg) { printMessage("FallingX"); }

static void onRisingEdge1(void *arg) {
  static int counter = 0;
  if (counter++ % 128 == 127) {
    printMessage("Rising1");
    counter = 0;
  }
}

static void onFallingEdge1(void *arg) {
  static int counter = 0;
  if (counter++ % 128 == 127) {
    printMessage("Falling1");
    counter = 0;
  }
}

static void onCallbackA(void *arg) {
  static int counter = 0;
  if (counter++ % 1024 == 1023) {
    printMessage("CallbackA");
    counter = 0;
  }
}

int main() {
  Clef::Impl::Atmega2560::serial.init();
  if (clock.init()) {
    Clef::Impl::Atmega2560::serial.writeLine(";;;;;;;;");
  }

  Clef::Impl::Atmega2560::xAxisTimer.init();
  Clef::Impl::Atmega2560::xAxisTimer.setDutyCycle(0.8f);
  Clef::Impl::Atmega2560::xAxisTimer.setFrequency(1.0f);
  Clef::Impl::Atmega2560::xAxisTimer.setRisingEdgeCallback(onRisingEdgeX,
                                                           nullptr);
  Clef::Impl::Atmega2560::xAxisTimer.setFallingEdgeCallback(onFallingEdgeX,
                                                            nullptr);
  Clef::Impl::Atmega2560::xAxisTimer.enable();

  Clef::Impl::Atmega2560::timer1.init();
  Clef::Impl::Atmega2560::timer1.setDutyCycle(0.8f);
  Clef::Impl::Atmega2560::timer1.setFrequency(100.0f);
  Clef::Impl::Atmega2560::timer1.setRisingEdgeCallback(onRisingEdge1, nullptr);
  Clef::Impl::Atmega2560::timer1.setFallingEdgeCallback(onFallingEdge1,
                                                        nullptr);
  Clef::Impl::Atmega2560::timer1.enable();

  Clef::Impl::Atmega2560::timer2.init();
  Clef::Impl::Atmega2560::timer2.setDutyCycleA(0.8f);
  Clef::Impl::Atmega2560::timer2.setDutyCycleB(0.4f);
  Clef::Impl::Atmega2560::timer2.setCallbackTop(onCallbackA, nullptr);
  Clef::Impl::Atmega2560::timer2.enable();

  while (1) {
    // Idle loop
  }
}
