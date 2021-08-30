// Copyright 2021 by Daniel Winkelman. All rights reserved.

#pragma once

/**
 * Number of microsteps per millimeter of each axis.
 */
#define USTEPS_PER_MM_X 160L
#define USTEPS_PER_MM_Y 160L
#define USTEPS_PER_MM_Z 400L
#define USTEPS_PER_MM_E 1280L

/**
 * Set a limit on stepper motor pulse frequency.
 */
#define MAX_STEPPER_FREQ 16000.0f
