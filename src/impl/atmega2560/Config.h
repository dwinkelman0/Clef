// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

#include <fw/Config.h>

/**
 * General configuration for the Atmega2560 chip in an Arduino.
 */
#ifndef F_CPU
#define F_CPU 16000000L
#endif

// Choose a low-ish frequency because there is something weird with the library
// that causes it to drop bits while receiving.
#define SERIAL_BAUDRATE 57600
#define MASS_SENSOR_SERIAL_BAUDRATE 9600
