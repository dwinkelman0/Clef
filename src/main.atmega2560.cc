// Copyright 2021 by Daniel Winkelman. All rights reserved.

#include <fw/Action.h>
#include <fw/GcodeParser.h>
#include <if/Interrupts.h>
#include <impl/atmega2560/Clock.h>
#include <impl/atmega2560/PwmTimer.h>
#include <impl/atmega2560/Register.h>
#include <impl/atmega2560/SensorInput.h>
#include <impl/atmega2560/Serial.h>
#include <impl/atmega2560/Stepper.h>
#include <stdio.h>

Clef::Impl::Atmega2560::Clock clock(Clef::Impl::Atmega2560::clockTimer);
Clef::Fw::DisplacementSensor<USTEPS_PER_MM_DISPLACEMENT, USTEPS_PER_MM_E>
    displacementSensor(clock, 0.1);
Clef::Fw::ActionQueue actionQueue;
Clef::Fw::XYEPositionQueue xyePositionQueue;
Clef::Fw::GcodeParser gcodeParser;
Clef::Fw::Axes::XAxis xAxis(Clef::Impl::Atmega2560::xAxisStepper,
                            Clef::Impl::Atmega2560::xAxisTimer);
Clef::Fw::Axes::YAxis yAxis(Clef::Impl::Atmega2560::yAxisStepper,
                            Clef::Impl::Atmega2560::yAxisTimer);
Clef::Fw::Axes::ZAxis zAxis(Clef::Impl::Atmega2560::zAxisStepper,
                            Clef::Impl::Atmega2560::zeAxisTimer);
Clef::Fw::Axes::EAxis eAxis(Clef::Impl::Atmega2560::eAxisStepper,
                            Clef::Impl::Atmega2560::zeAxisTimer,
                            displacementSensor);
Clef::Fw::Axes axes(xAxis, yAxis, zAxis, eAxis);
Clef::Fw::Context context({axes, gcodeParser, clock,
                           Clef::Impl::Atmega2560::serial, actionQueue});

void status(void *arg) {
  static int counter = 0;
  if (counter++ % 64 == 0) {
    Clef::If::EnableInterrupts interrupts;
    char buffer[64];
    sprintf(buffer, "Position = (%ld, %ld, %ld, %ld)",
            *axes.getX().getPosition(), *axes.getY().getPosition(),
            *axes.getZ().getPosition(), *axes.getE().getPosition());
    Clef::Impl::Atmega2560::serial.writeLine(buffer);
  }
}

void checkDisplacementSensor() {
  if (displacementSensor.checkOut()) {
    Clef::Util::Position<float, Clef::Util::PositionUnit::USTEP,
                         USTEPS_PER_MM_E>
        position = displacementSensor.readPosition();
    Clef::Util::Feedrate<float, Clef::Util::PositionUnit::USTEP,
                         Clef::Util::TimeUnit::MIN, USTEPS_PER_MM_E>
        feedrate = displacementSensor.readFeedrate();
    char buffer[64];
    sprintf(buffer, "Received data: x = %ld, v = %ld",
            static_cast<uint32_t>(*position), static_cast<uint32_t>(*feedrate));
    Clef::Impl::Atmega2560::serial.writeLine(buffer);
    displacementSensor.release();
  }
}

void startSpiRead(void *arg) {
  static int counter = 0;
  if (counter++ % 64 == 0) {
    Clef::Impl::Atmega2560::spi.initRead(4, 3);
  }
}

void onSpiReadComplete(const uint16_t size, const char *const data, void *arg) {
  Clef::If::EnableInterrupts interrupts;
  char buffer[64];
  sprintf(buffer, "Received data: %x, %x, %x, %x", data[0], data[1], data[2],
          data[3]);
  Clef::Impl::Atmega2560::serial.writeLine(buffer);
}

int main() {
  Clef::Impl::Atmega2560::serial.init();
  if (clock.init()) {
    Clef::Impl::Atmega2560::serial.writeLine("Initialized clock");
  }
  axes.init();

  Clef::Impl::Atmega2560::extruderCaliper.init();
  Clef::Impl::Atmega2560::extruderCaliper.setConversionCallback(
      Clef::Fw::DisplacementSensor<USTEPS_PER_MM_DISPLACEMENT,
                                   USTEPS_PER_MM_E>::injectWrapper,
      &displacementSensor);

  Clef::Impl::Atmega2560::spi.init();
  Clef::Impl::Atmega2560::spi.setReadCompleteCallback(onSpiReadComplete,
                                                      nullptr);

  Clef::Impl::Atmega2560::timer1.init();
  Clef::Impl::Atmega2560::timer1.setFrequency(256.0f);
  // Clef::Impl::Atmega2560::timer1.setRisingEdgeCallback(status, nullptr);
  Clef::Impl::Atmega2560::timer1.setFallingEdgeCallback(startSpiRead, nullptr);
  Clef::Impl::Atmega2560::timer1.enable();

  Clef::Fw::ActionQueue::Iterator it = actionQueue.first();
  int currentQueueSize = actionQueue.size();
  while (1) {
    gcodeParser.ingest(context);
    checkDisplacementSensor();
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
      sprintf(buffer, "Queue size = %d", newQueueSize);
      Clef::Impl::Atmega2560::serial.writeLine(buffer);
      currentQueueSize = newQueueSize;
    }
  }
}
