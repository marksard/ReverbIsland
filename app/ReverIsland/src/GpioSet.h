/*!
 * gpio setting
 * Copyright 2023 marksard
 * This software is released under the MIT license.
 * see https://opensource.org/licenses/MIT
 */ 

#pragma once

#include <Arduino.h>

// #define rev100
// #define PROTO

// GIPO割り当て
#define SW0 15
#define SW1 14
#define POT0 A0
#define POT1 A1
#define POT2 A2
#define PWM_POT0 10
#define PWM_POT1 11
#define PWM_POT2 12
#define S0 1
#define S1 2
#define S2 3
#define T0 13

#ifdef rev100
#define ROM1 6
#define ROM2 7
#else
#define ROM1 0
#define ROM2 8
#endif

#define CV A3

#define POTS_MAX 3
#define POTS_BIT 12
#define POTS_MAX_VALUE 4095
