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

static Clef::Impl::Atmega2560::Clock clock(Clef::Impl::Atmega2560::clockTimer);
static Clef::Fw::TemperatureSensor temperatureSensor1(clock, 10e3, 7.4e3);
static Clef::Fw::TemperatureSensor temperatureSensor2(clock, 10e3, 7.4e3);

static void onConversion(uint16_t value, void *arg) {
  Clef::Fw::TemperatureSensor *sensor =
      reinterpret_cast<Clef::Fw::TemperatureSensor *>(arg);
  float ratio = value / 1024.0f;
  Clef::Impl::Atmega2560::timer2.setDutyCycleA(ratio);
  Clef::Impl::Atmega2560::timer2.setDutyCycleB(ratio);
  sensor->injectWrapper(ratio, sensor);
}

static void loopProcess(Clef::Fw::TemperatureSensor &sensor,
                        const uint8_t token, const uint8_t index) {
  static uint16_t count = 0;
  if (sensor.checkOut(token)) {
    float temperature = sensor.read().data;
    if (count++ % 256 == 0) {
      char buffer[64];
      sprintf(buffer, "temp %d: %d", index,
              static_cast<int16_t>(temperature * 100));
      Clef::Impl::Atmega2560::serial.writeLine(buffer);
    }
    sensor.release(token);
  }
}

int main() {
  Clef::Impl::Atmega2560::serial.init();
  if (clock.init()) {
    Clef::Impl::Atmega2560::serial.writeLine(";;;;;;;;");
  }

  Clef::Impl::Atmega2560::analogBank.init();
  Clef::Impl::Atmega2560::analogBank.addInput(0, onConversion,
                                              &temperatureSensor1);
  Clef::Impl::Atmega2560::analogBank.addInput(1, onConversion,
                                              &temperatureSensor2);

  Clef::Impl::Atmega2560::timer2.init();
  Clef::Impl::Atmega2560::timer2.setCallbackTop(
      Clef::Impl::Atmega2560::AnalogBank::onPwmTimerEdge,
      &Clef::Impl::Atmega2560::analogBank);
  Clef::Impl::Atmega2560::timer2.enable();

  uint8_t token1 = temperatureSensor1.subscribe();
  uint8_t token2 = temperatureSensor2.subscribe();
  while (1) {
    loopProcess(temperatureSensor1, token1, 1);
    loopProcess(temperatureSensor2, token2, 2);
  }
}