#include "morse_decoder.h"

// --------------------------------------------------
// Morse lookup table
// --------------------------------------------------
//
// MORSE_CODES[i] corresponds to LETTERS[i].
//
// Example:
// MORSE_CODES[0] = ".-"
// LETTERS[0]     = 'A'
//
// This lets us decode Morse by searching MORSE_CODES
// and returning the letter at the same index.
//

const char* MORSE_CODES[] =
{
  ".-",    // A
  "-...",  // B
  "-.-.",  // C
  "-..",   // D
  ".",     // E
  "..-.",  // F
  "--.",   // G
  "....",  // H
  "..",    // I
  ".---",  // J
  "-.-",   // K
  ".-..",  // L
  "--",    // M
  "-.",    // N
  "---",   // O
  ".--.",  // P
  "--.-",  // Q
  ".-.",   // R
  "...",   // S
  "-",     // T
  "..-",   // U
  "...-",  // V
  ".--",   // W
  "-..-",  // X
  "-.--",  // Y
  "--.."   // Z
};

// Matching letter table.
// Each letter lines up with the Morse code at the same index.
const char LETTERS[] =
{
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
  'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
};

// Number of Morse code entries in the lookup table.
const int NUM_MORSE_CODES = 26;

// --------------------------------------------------
// decodeMorse
// --------------------------------------------------
//
// Takes one Morse character, such as ".-" or "...",
// and returns the corresponding English letter.
//
// Example:
// decodeMorse(".-")  returns 'A'
// decodeMorse("...") returns 'S'
//
// If the Morse pattern is not found, it returns '?'.
//

char decodeMorse(String morse)
{
  // Search through every Morse code in the table.
  for (int i = 0; i < NUM_MORSE_CODES; i++)
  {
    // Arduino String comparison checks the text contents.
    // If the input matches this table entry, return the matching letter.
    if (morse == MORSE_CODES[i])
    {
      return LETTERS[i];
    }
  }

  // If no matching Morse code was found, return '?'.
  return '?';
}

// --------------------------------------------------
// translateMorseString
// --------------------------------------------------
//
// Takes a full raw Morse message and converts it into text.
//
// Raw Morse format used by this project:
//
// Single space separates letters.
// Slash separates words.
//
// Example:
// "... --- ..."         becomes "SOS"
// "... --- ... / ..."   becomes "SOS S"
//
// The function reads the Morse message one character at a time,
// builds one Morse letter at a time, decodes it, then adds it
// to translatedText.
//

String translateMorseString(String morseMessage)
{
  // Final translated English output.
  String translatedText = "";

  // Temporarily stores one Morse letter while reading the message.
  //
  // Example:
  // As the function reads '.', '.', '.', currentMorseCharacter becomes "..."
  // When it reaches a space, "..." gets decoded into 'S'.
  String currentMorseCharacter = "";

  // Loop through each character in the raw Morse message.
  for (int i = 0; i < morseMessage.length(); i++)
  {
    char currentChar = morseMessage[i];

    // A space means the current Morse letter is complete.
    if (currentChar == ' ')
    {
      // Only decode if we actually have dots/dashes stored.
      // This avoids decoding empty spaces.
      if (currentMorseCharacter.length() > 0)
      {
        char decodedChar = decodeMorse(currentMorseCharacter);

        translatedText += decodedChar;

        // Clear the buffer so we can start building the next Morse letter.
        currentMorseCharacter = "";
      }
    }

    // A slash means a word gap.
    else if (currentChar == '/')
    {
      // If there is an unfinished Morse character before the slash,
      // decode it before adding the word space.
      if (currentMorseCharacter.length() > 0)
      {
        char decodedChar = decodeMorse(currentMorseCharacter);

        translatedText += decodedChar;

        currentMorseCharacter = "";
      }

      // Add a normal space between translated words.
      translatedText += " ";
    }

    // Otherwise, the character should be part of a Morse symbol.
    // It should be either '.' or '-'.
    else
    {
      currentMorseCharacter += currentChar;
    }
  }

  // If the raw Morse message does not end with a space or slash,
  // there may still be one final Morse character left to decode.
  if (currentMorseCharacter.length() > 0)
  {
    char decodedChar = decodeMorse(currentMorseCharacter);

    translatedText += decodedChar;
  }

  return translatedText;
}