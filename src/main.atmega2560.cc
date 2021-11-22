// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include <fw/Action.h>
#include <fw/GcodeParser.h>
#include <if/Interrupts.h>
#include <impl/atmega2560/AnalogBank.h>
#include <impl/atmega2560/Clock.h>
#include <impl/atmega2560/LimitSwitch.h>
#include <impl/atmega2560/PwmTimer.h>
#include <impl/atmega2560/Register.h>
#include <impl/atmega2560/SensorInput.h>
#include <impl/atmega2560/Serial.h>
#include <impl/atmega2560/Stepper.h>
#include <stdio.h>

Clef::Impl::Atmega2560::Clock clock(Clef::Impl::Atmega2560::clockTimer);
Clef::Fw::DisplacementSensor<USTEPS_PER_MM_DISPLACEMENT, USTEPS_PER_MM_E>
    displacementSensor(clock, 0.1);
Clef::Fw::PressureSensor pressureSensor(clock, 1);
Clef::Fw::TemperatureSensor syringeTemperatureSensor(clock, 10e3, 7.4e3);
Clef::Fw::TemperatureSensor needleTemperatureSensor(clock, 10e3, 7.4e3);
Clef::Fw::Heater syringeHeater(syringeTemperatureSensor,
                               Clef::Impl::Atmega2560::timer2,
                               &Clef::If::DirectOutputPwmTimer::setDutyCycleA,
                               0.01f, 0.002f, 0.0f);
Clef::Fw::Heater needleHeater(needleTemperatureSensor,
                              Clef::Impl::Atmega2560::timer2,
                              &Clef::If::DirectOutputPwmTimer::setDutyCycleB,
                              0.01f, 0.002f, 0.0f);
Clef::Fw::ActionQueue actionQueue;
Clef::Fw::XYEPositionQueue xyePositionQueue;
Clef::Fw::GcodeParser gcodeParser;
Clef::Fw::KalmanFilterExtrusionPredictor extrusionPredictor;
Clef::Fw::Axes::XAxis xAxis(Clef::Impl::Atmega2560::xAxisStepper,
                            Clef::Impl::Atmega2560::xAxisTimer);
Clef::Fw::Axes::YAxis yAxis(Clef::Impl::Atmega2560::yAxisStepper,
                            Clef::Impl::Atmega2560::yAxisTimer);
Clef::Fw::Axes::ZAxis zAxis(Clef::Impl::Atmega2560::zAxisStepper,
                            Clef::Impl::Atmega2560::zeAxisTimer);
Clef::Fw::Axes::EAxis eAxis(Clef::Impl::Atmega2560::eAxisStepper,
                            Clef::Impl::Atmega2560::zeAxisTimer,
                            displacementSensor, pressureSensor,
                            extrusionPredictor, syringeHeater, needleHeater);
Clef::Fw::Axes axes(xAxis, yAxis, zAxis, eAxis);
Clef::Fw::Context context({axes, gcodeParser, clock,
                           Clef::Impl::Atmega2560::serial, actionQueue,
                           xyePositionQueue});

void status(void *arg) {
  static int counter = 0;
  if (counter++ % 64 == 0) {
    Clef::If::EnableInterrupts interrupts;
    char buffer[64];
    sprintf(buffer, ";Position = (%ld, %ld, %ld, %ld)",
            *axes.getX().getPosition(), *axes.getY().getPosition(),
            *axes.getZ().getPosition(), *axes.getE().getPosition());
    // Clef::Impl::Atmega2560::serial.writeLine(buffer);
  }
}

void extruderStatus(void *arg) {
  static int counter = 0;
  if (counter++ % 10 == 0) {
    Clef::If::EnableInterrupts interrupts;
    typename Clef::Fw::Axes::EAxis::StepperPosition position =
        axes.getE().getPosition();
    Clef::Util::Time<uint64_t, Clef::Util::TimeUnit::USEC> time =
        clock.getMicros();
    char buffer[64];
    sprintf(buffer, ",xe=%ld", static_cast<uint32_t>(*position));
    Clef::Impl::Atmega2560::serial1.writeStr(";t=");
    Clef::Impl::Atmega2560::serial1.writeUint64(*time);
    Clef::Impl::Atmega2560::serial1.writeLine(buffer);
  }
}

void checkSensors(const uint8_t displacementSensorToken,
                  const uint8_t pressureSensorToken,
                  const uint8_t syringeTemperatureSensorToken,
                  const uint8_t needleTemperatureSensorToken) {
  if (displacementSensor.isSampleReady(displacementSensorToken) &&
      pressureSensor.isSampleReady(pressureSensorToken) &&
      syringeTemperatureSensor.isSampleReady(syringeTemperatureSensorToken) &&
      needleTemperatureSensor.isSampleReady(needleTemperatureSensorToken)) {
    displacementSensor.checkOut(displacementSensorToken);
    pressureSensor.checkOut(pressureSensorToken);
    syringeTemperatureSensor.checkOut(syringeTemperatureSensorToken);
    needleTemperatureSensor.checkOut(needleTemperatureSensorToken);

    static bool initializedExtruder = false;
    if (!initializedExtruder) {
      eAxis.setDisplacementSensorOffset(displacementSensor.readPosition());
      initializedExtruder = true;
    }
    typename Clef::Fw::Axes::EAxis::StepperPosition extruderPosition =
        axes.getE().getPosition();
    Clef::Util::Position<float, Clef::Util::PositionUnit::USTEP,
                         USTEPS_PER_MM_E>
        sensorPosition = displacementSensor.readPosition();
    float pressure = pressureSensor.readPressure();
    float syringeTemp = syringeTemperatureSensor.read().data;
    float needleTemp = needleTemperatureSensor.read().data;
    Clef::Util::Time<uint64_t, Clef::Util::TimeUnit::USEC> time =
        displacementSensor.getMeasurementTime();
    char buffer[64];

    Clef::If::EnableInterrupts interrupts;
    Clef::Impl::Atmega2560::serial1.writeStr(";t=");
    Clef::Impl::Atmega2560::serial1.writeUint64(*time);
    sprintf(buffer, ",xe=%ld", static_cast<uint32_t>(*extruderPosition));
    Clef::Impl::Atmega2560::serial1.writeStr(buffer);
    sprintf(buffer, ",xs=%ld", static_cast<uint32_t>(*sensorPosition));
    Clef::Impl::Atmega2560::serial1.writeStr(buffer);
    sprintf(buffer, ",P=%ld", static_cast<int32_t>(pressure));
    Clef::Impl::Atmega2560::serial1.writeStr(buffer);
    sprintf(buffer, ",Ts=%ld", static_cast<int32_t>(syringeTemp * 100));
    Clef::Impl::Atmega2560::serial1.writeStr(buffer);
    sprintf(buffer, ",Tn=%ld", static_cast<int32_t>(needleTemp * 100));
    Clef::Impl::Atmega2560::serial1.writeLine(buffer);
    needleTemperatureSensor.release(needleTemperatureSensorToken);
    syringeTemperatureSensor.release(syringeTemperatureSensorToken);
    pressureSensor.release(pressureSensorToken);
    displacementSensor.release(displacementSensorToken);
  }
}

void startSpiRead(void *arg) { Clef::Impl::Atmega2560::spi.initRead(4, 20); }

void limitSwitchAction(void *arg, const uint8_t arg2) {
  Clef::If::EnableInterrupts interrupts;
  char buffer[64];
  sprintf(buffer, ";Limit switch %s", static_cast<const char *>(arg));
  Clef::Impl::Atmega2560::serial.writeLine(buffer);
}

int main() {
  Clef::Impl::Atmega2560::serial.init();
  Clef::Impl::Atmega2560::serial1.init();
  if (clock.init()) {
    Clef::Impl::Atmega2560::serial.writeLine(";;;;;;;;");
    Clef::Impl::Atmega2560::serial1.writeLine(";;;;;;;;");
  }
  Clef::Impl::Atmega2560::serial1.writeLine(";power_on");
  axes.init();

  Clef::Impl::Atmega2560::limitSwitches.init();
  Clef::Impl::Atmega2560::limitSwitches.getX().setTriggerCallback(
      limitSwitchAction, const_cast<char *>("X"), 0);
  Clef::Impl::Atmega2560::limitSwitches.getY().setTriggerCallback(
      limitSwitchAction, const_cast<char *>("Y"), 1);
  Clef::Impl::Atmega2560::limitSwitches.getZ().setTriggerCallback(
      limitSwitchAction, const_cast<char *>("Z"), 2);
  Clef::Impl::Atmega2560::limitSwitches.getEInc().setTriggerCallback(
      limitSwitchAction, const_cast<char *>("E+"), 3);
  Clef::Impl::Atmega2560::limitSwitches.getEDec().setTriggerCallback(
      limitSwitchAction, const_cast<char *>("E-"), 4);

  Clef::Impl::Atmega2560::extruderCaliper.init();
  Clef::Impl::Atmega2560::extruderCaliper.setConversionCallback(
      Clef::Fw::DisplacementSensor<USTEPS_PER_MM_DISPLACEMENT,
                                   USTEPS_PER_MM_E>::injectWrapper,
      &displacementSensor);
  uint8_t displacementSensorToken = displacementSensor.subscribe();

  Clef::Impl::Atmega2560::spi.init();
  Clef::Impl::Atmega2560::spi.setReadCompleteCallback(
      Clef::Fw::PressureSensor::injectWrapper, &pressureSensor);
  uint8_t pressureSensorToken = pressureSensor.subscribe();
  Clef::Impl::Atmega2560::timer1.setFallingEdgeCallback(startSpiRead, nullptr);

  Clef::Impl::Atmega2560::analogBank.init();
  Clef::Impl::Atmega2560::analogBank.addInput(
      0, Clef::Fw::TemperatureSensor::injectWrapper, &syringeTemperatureSensor);
  Clef::Impl::Atmega2560::analogBank.addInput(
      1, Clef::Fw::TemperatureSensor::injectWrapper, &needleTemperatureSensor);
  uint8_t syringeTemperatureSensorToken = syringeTemperatureSensor.subscribe();
  uint8_t needleTemperatureSensorToken = needleTemperatureSensor.subscribe();

  Clef::Impl::Atmega2560::timer2.init();
  Clef::Impl::Atmega2560::timer2.setCallbackTop(
      Clef::Impl::Atmega2560::AnalogBank::onPwmTimerEdge,
      &Clef::Impl::Atmega2560::analogBank);
  Clef::Impl::Atmega2560::timer2.enable();

  Clef::Fw::ActionQueue::Iterator it = actionQueue.first();
  int currentQueueSize = actionQueue.size();
  while (1) {
    gcodeParser.ingest(context);
    checkSensors(displacementSensorToken, pressureSensorToken,
                 syringeTemperatureSensorToken, needleTemperatureSensorToken);
    syringeHeater.onLoop();
    needleHeater.onLoop();
    checkSensors(displacementSensorToken, pressureSensorToken, 0, 0);
    if (it) {
      (*it)->onLoop(context);
      if ((*it)->isFinished(context)) {
        context.actionQueue.pop(context);
        if ((it = actionQueue.first())) {
          (*it)->onStart(context);
        }
      }
    } else {
      if ((it = actionQueue.first())) {
        (*it)->onStart(context);
      }
    }
    int newQueueSize = actionQueue.size();
    if (newQueueSize != currentQueueSize) {
      char buffer[64];
      sprintf(buffer, ";Queue size = %d", newQueueSize);
      Clef::Impl::Atmega2560::serial.writeLine(buffer);
      currentQueueSize = newQueueSize;
    }
  }
}
