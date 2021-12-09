#!/bin/bash
# Copyright 2021 by Daniel Winkelman. All rights reserved.

# Add this file to your .bashrc, e.g.:
#   source ~/Code/Clef/commands.sh

# CMake shortcut for compiling code for Arduino
# Add $CLEF_ARDUINO_PATH and $CLEF_PRESSURE_SENSOR to your variable environment
alias cmake-atmega2560="cmake -D CLEF_TARGET=atmega2560 -D CLEF_ARDUINO_PATH=$CLEF_ARDUINO_PATH -D CLEF_PORT=/dev/ttyACM0 -D CLEF_BAUD=115200 -D PRESSURE_SENSOR=$CLEF_PRESSURE_SENSOR"
