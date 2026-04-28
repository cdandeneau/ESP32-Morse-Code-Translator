#include <Arduino.h>
#include "config.h"
#include "morse_decoder.h"

// --------------------------------------------------
// Morse message storage
// --------------------------------------------------
//
// morseString stores the raw Morse input.
//
// Example:
// SOS SOS becomes:
// "... --- ... / ... --- ... "
//
// Single space separates letters.
// Slash separates words.
//

String morseString = "";

// These flags prevent the program from repeatedly adding
// spaces or word separators while the button remains released.
bool letterGapAdded = false;
bool wordGapAdded = false;

// --------------------------------------------------
// Debounce state for Morse input button
// --------------------------------------------------

int morseStableState = HIGH;
int morseLastRawState = HIGH;
unsigned long morseLastDebounceTime = 0;

// --------------------------------------------------
// Debounce state for print button
// --------------------------------------------------

int printStableState = HIGH;
int printLastRawState = HIGH;
unsigned long printLastDebounceTime = 0;

// --------------------------------------------------
// Timing variables for measuring button press/release
// --------------------------------------------------

unsigned long pressStartTime = 0;
unsigned long releaseStartTime = 0;

// --------------------------------------------------
// Function declarations
// --------------------------------------------------

void handleMorseButton();
void handlePrintButton();
void handleMorseTimeout();
void addMorseSymbol(unsigned long pressDuration);
void printCurrentMorse();

void setup()
{
  pinMode(LED_PIN, OUTPUT);
  pinMode(MORSE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(PRINT_BUTTON_PIN, INPUT_PULLUP);

  digitalWrite(LED_PIN, LOW);

  Serial.begin(115200);
  delay(2000);

  Serial.println("Morse Code Translator");
  Serial.println("GPIO18: Morse input button");
  Serial.println("GPIO19: Print translated Morse string");
  Serial.println();

  Serial.print("Time unit: ");
  Serial.print(TIME_UNIT);
  Serial.println(" ms");

  Serial.print("Dot/dash threshold: ");
  Serial.print(DOT_DASH_THRESHOLD);
  Serial.println(" ms");

  Serial.print("Letter timeout: ");
  Serial.print(LETTER_TIMEOUT);
  Serial.println(" ms");

  Serial.print("Word timeout: ");
  Serial.print(WORD_TIMEOUT);
  Serial.println(" ms");

  Serial.println();
}

void loop()
{
  // Check the Morse input button for dots/dashes.
  handleMorseButton();

  // Check whether enough release time has passed
  // to add a letter gap or word gap.
  handleMorseTimeout();

  // Check the print button.
  handlePrintButton();
}

void handleMorseButton()
{
  int morseRawState = digitalRead(MORSE_BUTTON_PIN);

  // If the raw reading changed, restart debounce timer.
  if (morseRawState != morseLastRawState)
  {
    morseLastDebounceTime = millis();
  }

  // Only trust the input after it has been stable
  // for the debounce delay.
  if ((millis() - morseLastDebounceTime) > DEBOUNCE_DELAY)
  {
    // If the stable state changed, then we have a clean press/release event.
    if (morseRawState != morseStableState)
    {
      morseStableState = morseRawState;

      // Button pressed.
      // INPUT_PULLUP means pressed = LOW.
      if (morseStableState == LOW)
      {
        digitalWrite(LED_PIN, HIGH);

        // Start measuring how long the button is held.
        pressStartTime = millis();

        // A new press means we are no longer waiting to add a gap.
        letterGapAdded = false;
        wordGapAdded = false;
      }

      // Button released.
      else
      {
        digitalWrite(LED_PIN, LOW);

        // Start measuring how long the button has been released.
        releaseStartTime = millis();

        // Calculate how long the button was held down.
        unsigned long pressDuration = millis() - pressStartTime;

        // Convert the press duration into a dot or dash.
        addMorseSymbol(pressDuration);
      }
    }
  }

  // Save raw state for next loop.
  morseLastRawState = morseRawState;
}

void addMorseSymbol(unsigned long pressDuration)
{
  Serial.print("Press duration: ");
  Serial.print(pressDuration);
  Serial.print(" ms -> ");

  // Less than 2 units is treated as a dot.
  // Greater than or equal to 2 units is treated as a dash.
  if (pressDuration < DOT_DASH_THRESHOLD)
  {
    Serial.println(".");
    morseString += ".";
  }

  else
  {
    Serial.println("-");
    morseString += "-";
  }
}

void handleMorseTimeout()
{
  // Only add gaps while the Morse button is released
  // and there is at least one Morse symbol stored.
  if (morseStableState == HIGH && morseString.length() > 0)
  {
    unsigned long releaseDuration = millis() - releaseStartTime;

    // After 3 units of release time, add a letter separator.
    if (releaseDuration >= LETTER_TIMEOUT && letterGapAdded == false)
    {
      morseString += " ";
      letterGapAdded = true;

      Serial.print("Letter gap added. Current Morse message: ");
      Serial.println(morseString);
    }

    // After 7 units of release time, convert the previous letter gap
    // into a word separator.
    if (releaseDuration >= WORD_TIMEOUT && wordGapAdded == false)
    {
      // If we already added a letter space, remove it before adding word gap.
      if (morseString.endsWith(" "))
      {
        morseString.remove(morseString.length() - 1);
      }

      morseString += " / ";
      wordGapAdded = true;

      Serial.print("Word gap added. Current Morse message: ");
      Serial.println(morseString);
    }
  }
}

void handlePrintButton()
{
  int printRawState = digitalRead(PRINT_BUTTON_PIN);

  // If the raw reading changed, restart debounce timer.
  if (printRawState != printLastRawState)
  {
    printLastDebounceTime = millis();
  }

  // Only trust the input after it has been stable
  // for the debounce delay.
  if ((millis() - printLastDebounceTime) > DEBOUNCE_DELAY)
  {
    if (printRawState != printStableState)
    {
      printStableState = printRawState;

      // Button pressed.
      if (printStableState == LOW)
      {
        printCurrentMorse();
      }
    }
  }

  // Save raw state for next loop.
  printLastRawState = printRawState;
}

void printCurrentMorse()
{
  String translatedText = translateMorseString(morseString);

  Serial.println();
  Serial.print("Raw Morse code: ");
  Serial.println(morseString);

  Serial.print("Translated text: ");
  Serial.println(translatedText);
  Serial.println();
}