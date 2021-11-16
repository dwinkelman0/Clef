// Copyright 2021 by Daniel Winkelman. All rights reserved.

/**
 * This test case makes sure PWM timers of both varieties work. For this test,
 * expect that Arduino Mega pins 9 and 10 produce waveforms and all other pins
 * do not.
 */

#include <if/Interrupts.h>
#include <impl/atmega2560/AnalogBank.h>
#include <impl/atmega2560/Clock.h>
#include <impl/atmega2560/PwmTimer.h>
#include <impl/atmega2560/Serial.h>
#include <stdio.h>

static Clef::Impl::Atmega2560::Clock clock(Clef::Impl::Atmega2560::clockTimer);

static void onConversion(uint16_t value, void *arg) {
  Clef::Impl::Atmega2560::timer2.setDutyCycleA(value / 1024.0f);
}

int main() {
  Clef::Impl::Atmega2560::serial.init();
  if (clock.init()) {
    Clef::Impl::Atmega2560::serial.writeLine(";;;;;;;;");
  }

  Clef::Impl::Atmega2560::analogBank.init();
  Clef::Impl::Atmega2560::analogBank.addInput(2, onConversion, nullptr);
  Clef::Impl::Atmega2560::analogBank.addInput(13, onConversion, nullptr);

  Clef::Impl::Atmega2560::timer2.init();
  Clef::Impl::Atmega2560::timer2.setCallbackTop(
      Clef::Impl::Atmega2560::AnalogBank::onPwmTimerEdge,
      &Clef::Impl::Atmega2560::analogBank);
  Clef::Impl::Atmega2560::timer2.enable();

  while (1) {
    // Idle loop
  }
}
