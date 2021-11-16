// Copyright 2021 by Daniel Winkelman. All rights reserved.

/**
 * This test case makes sure thermistors work. The voltage drop across a
 * thermistor is measured by analog input 0, and a PWM signal is generated on
 * pins 9 and 10. The temperature conversion is printed to serial.
 */

#include <fw/Sensor.h>
#include <if/Interrupts.h>
#include <impl/atmega2560/AnalogBank.h>
#include <impl/atmega2560/Clock.h>
#include <impl/atmega2560/PwmTimer.h>
#include <impl/atmega2560/Register.h>
#include <impl/atmega2560/Serial.h>
#include <stdio.h>

class Pin6 W_REGISTER_BOOL(H, 3, false);

static Clef::Impl::Atmega2560::Clock clock(Clef::Impl::Atmega2560::clockTimer);
static Clef::Fw::TemperatureSensor temperatureSensor(clock, 10e3, 11.9e3);

static void onConversion(uint16_t value, void *arg) {
  float ratio = value / 1024.0f;
  Clef::Impl::Atmega2560::timer2.setDutyCycleA(ratio);
  Clef::Impl::Atmega2560::timer2.setDutyCycleB(ratio);
  temperatureSensor.injectWrapper(ratio, &temperatureSensor);
}

int main() {
  Clef::Impl::Atmega2560::serial.init();
  if (clock.init()) {
    Clef::Impl::Atmega2560::serial.writeLine(";;;;;;;;");
  }

  Clef::Impl::Atmega2560::analogBank.init();
  Clef::Impl::Atmega2560::analogBank.addInput(0, onConversion, nullptr);

  Clef::Impl::Atmega2560::timer2.init();
  Clef::Impl::Atmega2560::timer2.setCallbackTop(
      Clef::Impl::Atmega2560::AnalogBank::onPwmTimerEdge,
      &Clef::Impl::Atmega2560::analogBank);
  Clef::Impl::Atmega2560::timer2.enable();

  uint8_t token = temperatureSensor.subscribe();
  uint16_t count = 0;
  while (1) {
    if (temperatureSensor.checkOut(token)) {
      float temperature = temperatureSensor.read().data;
      if (count++ % 256 == 0) {
        char buffer[64];
        sprintf(buffer, "temp: %d", static_cast<int16_t>(temperature * 100));
        Clef::Impl::Atmega2560::serial.writeLine(buffer);
      }
      temperatureSensor.release(token);
    }
  }
}
