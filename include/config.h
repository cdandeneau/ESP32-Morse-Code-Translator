#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Pin assignments
const int LED_PIN = 4;
const int MORSE_BUTTON_PIN = 18;
const int PRINT_BUTTON_PIN = 19;

// Morse timing
const unsigned long TIME_UNIT = 250;
const unsigned long DEBOUNCE_DELAY = 50;

const unsigned long DOT_DASH_THRESHOLD = 2 * TIME_UNIT;
const unsigned long LETTER_TIMEOUT = 3 * TIME_UNIT;
const unsigned long WORD_TIMEOUT = 7 * TIME_UNIT;

#endif