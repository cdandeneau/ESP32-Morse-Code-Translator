#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include "morse_decoder.h"

Adafruit_SSD1306 display(128, 64, &Wire, -1);

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
// Debounce state for play/clear button
// --------------------------------------------------
int playStableState = HIGH;
int playLastRawState = HIGH;
unsigned long playLastDebounceTime = 0;
unsigned long playClearPressStartTime = 0;

// --------------------------------------------------
// Timing variables for measuring button press/release
// --------------------------------------------------

unsigned long pressStartTime = 0;
unsigned long releaseStartTime = 0;

bool onInputScreen = true;
unsigned long lastCursorBlink = 0;

// --------------------------------------------------
// Function declarations
// --------------------------------------------------

void playCurrentMorse();
void handlePlay_ClearStringButton();
void handleMorseButton();
void handlePrintButton();
void handleMorseTimeout();
void addMorseSymbol(unsigned long pressDuration);
void printCurrentMorse();
void oledShowMorseInput();
void oledScrollText(String text);
void oledShowPrint(String morse, String translated);
void oledShowCleared();
void handleCursorBlink();

void setup()
{
  pinMode(LED_PIN, OUTPUT);
  pinMode(MORSE_BUTTON_PIN, INPUT_PULLUP);
  pinMode(PRINT_BUTTON_PIN, INPUT_PULLUP);
  pinMode(PLAY_CLEAR_BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(LED_PIN, LOW);
  ledcSetup(0, 1000, 8);
  ledcAttachPin(BUZZER_PIN, 0);

  Wire.begin(OLED_SDA, OLED_SCL);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Morse Translator");
  display.println("Ready.");
  display.display();

  Serial.begin(115200);
  delay(2000);

  Serial.println("Morse Code Translator");
  Serial.println("GPIO18: Morse input button");
  Serial.println("GPIO19: Print translated Morse string");
  Serial.println("GPIO22: Short press = play Morse, long press (1s) = clear");
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

  // Check the play button.
  handlePlay_ClearStringButton();

  // Blink the cursor on the input screen.
  handleCursorBlink();
}

void handlePlay_ClearStringButton()
{
  int playRawState = digitalRead(PLAY_CLEAR_BUTTON_PIN);

  // If the raw reading changed, restart debounce timer.
  if (playRawState != playLastRawState)
  {
    playLastDebounceTime = millis();
  }

  // Only trust the input after it has been stable
  // for the debounce delay.
  if ((millis() - playLastDebounceTime) > DEBOUNCE_DELAY)
  {
    if (playRawState != playStableState)
    {
      playStableState = playRawState;

      // Button pressed — record start time.
      if (playStableState == LOW)
      {
        playClearPressStartTime = millis();
      }

      // Button released — short press plays, long press clears.
      else
      {
        unsigned long holdDuration = millis() - playClearPressStartTime;

        if (holdDuration >= LONG_PRESS_THRESHOLD)
        {
          morseString = "";
          letterGapAdded = false;
          wordGapAdded = false;
          Serial.println("Morse string cleared.");
          oledShowCleared();
        }
        else
        {
          playCurrentMorse();
        }
      }
    }
  }

  // Save raw state for next loop.
  playLastRawState = playRawState;
}

void playCurrentMorse()
{
  String translated = translateMorseString(morseString);

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Playing...");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  display.setTextWrap(true);
  display.setCursor(0, 14);
  String shortMorse = morseString.length() > 42 ? morseString.substring(morseString.length() - 42) : morseString;
  display.print(shortMorse);
  display.setTextWrap(false);
  display.setCursor(0, 48);
  display.print("-> " + translated);
  display.display();

  Serial.println("Playing current Morse message...");
  for(int i = 0; i < morseString.length(); i++)
  {
    char symbol = morseString.charAt(i);

    if (symbol == '.')
    {
      ledcWriteTone(0, 1000); // Dot: 1 unit of tone
      delay(TIME_UNIT);
      ledcWrite(0, 0);
    }

    else if (symbol == '-')
    {
      ledcWriteTone(0, 1000); // Dash: 3 units of tone
      delay(3 * TIME_UNIT);
      ledcWrite(0, 0);
    }

    else if (symbol == '/')
    {
      delay(WORD_TIMEOUT); // Word gap: 7 units of silence
      continue; // skip the inter-symbol gap below
    }

    else if (symbol == ' ')
    {
      // Spaces adjacent to '/' are part of the word separator — skip them.
      bool adjacentToSlash = (i > 0 && morseString.charAt(i - 1) == '/')
                          || (i + 1 < (int)morseString.length() && morseString.charAt(i + 1) == '/');
      if (!adjacentToSlash)
        delay(LETTER_TIMEOUT); // Letter gap: 3 units of silence
      continue;
    }

    // Only add the inter-symbol gap when the next character is another dot/dash.
    // Skipping it before spaces and slashes keeps letter/word gap durations exact.
    if (i + 1 < (int)morseString.length())
    {
      char next = morseString.charAt(i + 1);
      if (next == '.' || next == '-')
        delay(TIME_UNIT);
    }
  }

  oledScrollText(translated);
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
        ledcWriteTone(0, 1000); // Start buzzer tone at 1kHz
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
        ledcWrite(0, 0); // Stop buzzer tone
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

  oledShowMorseInput();
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
      oledShowMorseInput();
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
      oledShowMorseInput();
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

void oledShowMorseInput()
{
  onInputScreen = true;
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Morse Input:");
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  String displayStr = morseString;
  displayStr.replace(" / ", "\n");
  if ((millis() / 500) % 2 == 0)
    displayStr += "|";

  // Break displayStr into lines: split on \n (word gaps) then wrap at 21 chars.
  // The text area is y=14 to y=55 — exactly 5 lines at 8px each.
  // If there are more than 5 lines, show only the last 5 (most recent input).
  const int MAX_LINES = 5;
  const int CHARS_PER_LINE = 21;
  String lines[20];
  int lineCount = 0;

  int segStart = 0;
  for (int i = 0; i <= (int)displayStr.length(); i++)
  {
    if (i == (int)displayStr.length() || displayStr[i] == '\n')
    {
      String seg = displayStr.substring(segStart, i);
      while (seg.length() > CHARS_PER_LINE && lineCount < 20)
      {
        lines[lineCount++] = seg.substring(0, CHARS_PER_LINE);
        seg = seg.substring(CHARS_PER_LINE);
      }
      if (lineCount < 20)
        lines[lineCount++] = seg;
      segStart = i + 1;
    }
  }

  int firstLine = lineCount > MAX_LINES ? lineCount - MAX_LINES : 0;
  display.setTextWrap(false);
  display.setCursor(0, 14);
  for (int i = firstLine; i < lineCount; i++)
    display.println(lines[i]);

  display.setCursor(0, 56);
  display.print("Building...");
  display.display();
}

// Scrolls text in large font from right to left across the bottom of the screen.
void oledScrollText(String text)
{
  int pixelWidth = (int)text.length() * 12; // size-2 font: 12px per char
  display.setTextWrap(false);
  for (int x = 128; x > -pixelWidth; x -= 2)
  {
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Translated:");
    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
    display.setTextSize(2);
    display.setCursor(x, 28);
    display.print(text);
    display.display();
    delay(20);
  }
}

void oledShowPrint(String morse, String translated)
{
  onInputScreen = false;
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  String displayMorse = morse.length() > 21 ? morse.substring(morse.length() - 21) : morse;
  display.println(displayMorse);
  display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
  display.setCursor(0, 14);
  display.println("Translated:");
  display.setTextSize(2);
  display.setCursor(0, 26);
  String displayTranslated = translated.length() > 10 ? translated.substring(0, 9) + "~" : translated;
  display.print(displayTranslated);
  display.display();
}

void oledShowCleared()
{
  onInputScreen = false;
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(22, 24);
  display.println("<< Cleared >>");
  display.setCursor(16, 40);
  display.println("Ready for input.");
  display.display();
}

void handleCursorBlink()
{
  if (!onInputScreen) return;
  if (millis() - lastCursorBlink >= 250)
  {
    lastCursorBlink = millis();
    oledShowMorseInput();
  }
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

  oledShowPrint(morseString, translatedText);
}