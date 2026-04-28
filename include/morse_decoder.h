#ifndef MORSE_DECODER_H
#define MORSE_DECODER_H

#include <Arduino.h>

char decodeMorse(String morse);
String translateMorseString(String morseMessage);

#endif