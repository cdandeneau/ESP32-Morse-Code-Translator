# ESP32 Morse Code Translator

A button-based Morse code translator built on an ESP32-WROVER board using PlatformIO and the Arduino framework.

## Features

- GPIO button input using internal pull-up resistors
- Software debouncing with `millis()`
- Dot/dash detection based on press duration
- Standard Morse timing ratios:
  - Dot: 1 unit
  - Dash: 3 units
  - Letter gap: 3 units
  - Word gap: 7 units
- Morse-to-English translation using a lookup table
- Serial monitor output for debugging and translated text
- LED feedback while entering Morse input

## Hardware

- ESP32-WROVER development board
- Pushbutton on GPIO18 for Morse input
- Pushbutton on GPIO19 for printing translated output
- LED on GPIO4 with current-limiting resistor

## Pinout

| Function           |     ESP32 GPIO   |
=========================================
| Morse input button |       GPIO18     |
| Print button       |       GPIO19     |
| LED output         |        GPIO4     |

## Usage

- Short press GPIO18 button to enter a dot.
- Long press GPIO18 button to enter a dash.
- Pause for one letter gap to separate letters.
- Pause for one word gap to separate words.
- Press GPIO19 button to print the translated message.

## Example

Raw Morse:

```text
... --- ... / ... --- ...

Translates to : SOS SOS