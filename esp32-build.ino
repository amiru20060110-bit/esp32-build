#include <Arduino.h>
#include "synas4.h"   // AS4 recorded WAV
#include "sync5.h"    // C5 recorded WAV
#include "synds4.h"   // D#4 recorded WAV

// Pin setup for 4x4 matrix (row -> column diode orientation)
const uint8_t rowPins[4] = {15, 16, 17, 18};  // Rows
const uint8_t colPins[4] = {21, 22, 23, 24};  // Columns

// Audio configuration
#define SAMPLE_RATE 44100
#define VOLUME 12000

// Struct for a key
struct Key {
  const byte* wav;    // Pointer to WAV array in PROGMEM
  float pitch;        // Pitch multiplier for pitch-shifted keys
};

// Full octave: C4 → C5 (white + black keys, 13 keys)
Key keys[13] = {
  {synas4, 0.667f},   // C4 (pitch-shifted from AS4)
  {synas4, 0.707f},   // C#4 / Db4
  {synas4, 0.75f},    // D4
  {synds4, 1.0f},     // D#4 (already recorded)
  {synas4, 0.84f},    // E4
  {synas4, 0.889f},   // F4
  {synas4, 0.943f},   // F#4 / Gb4
  {sync5, 0.933f},    // G4 (pitch-shifted from C5)
  {synas4, 1.0f},     // G#4 / Ab4
  {sync5, 1.0f},      // A4 (pitch-shifted from C5)
  {synds4, 1.25f},    // A#4 (already recorded)
  {sync5, 1.122f},    // B4
  {sync5, 1.189f}     // C5 (already recorded)
};

// Variables for scanning matrix
bool keyState[4][4];

void setup() {
  Serial.begin(115200);

  // Configure row pins as OUTPUT
  for (int i = 0; i < 4; i++) {
    pinMode(rowPins[i], OUTPUT);
    digitalWrite(rowPins[i], HIGH); // idle HIGH
  }

  // Configure column pins as INPUT_PULLUP
  for (int i = 0; i < 4; i++) {
    pinMode(colPins[i], INPUT_PULLUP);
  }

  Serial.println("Keyboard ready!");
}

void loop() {
  for (int row = 0; row < 4; row++) {
    digitalWrite(rowPins[row], LOW);  // Activate row
    for (int col = 0; col < 4; col++) {
      bool pressed = digitalRead(colPins[col]) == LOW;

      if (pressed && !keyState[row][col]) {
        keyState[row][col] = true;

        // Map row/col to key index (0-12)
        int keyIndex = row * 4 + col;
        if (keyIndex < 13) {
          playWav(keys[keyIndex].wav, keys[keyIndex].pitch);
        }
      } else if (!pressed && keyState[row][col]) {
        keyState[row][col] = false;
      }
    }
    digitalWrite(rowPins[row], HIGH);  // Deactivate row
  }
}

// Dummy playback function for example
// You need to use your WAV playback method here
void playWav(const byte* wav, float pitch) {
  // Example: just print info
  Serial.print("Play WAV at pitch: ");
  Serial.println(pitch);
  // Actual playback code goes here
  // Use I2S or DAC to output audio from PROGMEM
}