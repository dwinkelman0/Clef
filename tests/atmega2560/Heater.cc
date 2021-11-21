// Copyright 2021 by Daniel Winkelman. All rights reserved.

/**
 * Test that PID-controlled heaters work correctly. Pin 10 controls heater 1
 * (analog pin 0), and pin 9 controls heater 2 (analog pin 1).
 */

#include <fw/Heater.h>
#include <fw/PidController.h>
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
static Clef::Fw::Heater heater1(temperatureSensor1,
                                Clef::Impl::Atmega2560::timer2,
                                &Clef::If::DirectOutputPwmTimer::setDutyCycleA,
                                0.01f, 0.002f, 0.0f);
static Clef::Fw::Heater heater2(temperatureSensor2,
                                Clef::Impl::Atmega2560::timer2,
                                &Clef::If::DirectOutputPwmTimer::setDutyCycleB,
                                0.01f, 0.002f, 0.0f);

static void onConversion(uint16_t value, void *arg) {
  Clef::Fw::TemperatureSensor *sensor =
      reinterpret_cast<Clef::Fw::TemperatureSensor *>(arg);
  sensor->injectWrapper(value / 1024.0f, sensor);
}

static void loopProcess(Clef::Fw::TemperatureSensor &sensor,
                        const uint8_t token, const uint8_t index) {
  static uint16_t count = 0;
  if (sensor.checkOut(token)) {
    float temperature = sensor.read().data;
    if (count % 256 == 0 || count % 256 == 127) {
      char buffer[64];
      sprintf(buffer, "temp %d: %d", index,
              static_cast<int16_t>(temperature * 100));
      Clef::Impl::Atmega2560::serial.writeLine(buffer);
    }
    count++;
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

  heater1.setTarget(30.0f);
  heater2.setTarget(30.0f);

  uint8_t token1 = temperatureSensor1.subscribe();
  uint8_t token2 = temperatureSensor2.subscribe();
  while (1) {
    loopProcess(temperatureSensor1, token1, 1);
    heater1.onLoop();
    loopProcess(temperatureSensor2, token2, 2);
    heater2.onLoop();
  }
}
