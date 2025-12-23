#include <Arduino.h>
#include <SPI.h>
#include <SdFat.h>
#include <Audio.h> // ESP32 Audio library

// SD card pins
#define SD_MOSI 6
#define SD_MISO 5
#define SD_CLK  7
#define SD_CS   4

// Key matrix pins (1 key test)
#define ROW_PIN 15 // row pin
#define COL_PIN 40 // column pin

// I2S output pins
#define I2S_BCK  1
#define I2S_LRC  2
#define I2S_DOUT 3

// WAV file to test
const char *wavFile = "/ce4.wav"; // place TEST.wav in SD root

SdFat SD;
File audioFile;

// Audio object
Audio audio;

void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(ROW_PIN, INPUT_PULLUP);   // row pulled HIGH normally
  pinMode(COL_PIN, OUTPUT);         // column driven LOW to scan
  digitalWrite(COL_PIN, HIGH);

  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS, SPI, SD_SCK_MHZ(20))) {
    Serial.println("SD init failed!");
    while (1);
  }
  Serial.println("SD initialized.");

  // Open WAV file
  audioFile = SD.open(wavFile);
  if (!audioFile) {
    Serial.println("Failed to open WAV file!");
    while (1);
  }
  Serial.println("WAV file ready.");

  // Initialize I2S output
  audio.setPinout(I2S_BCK, I2S_LRC, I2S_DOUT);
  audio.begin(audioFile, 32000); // 32kHz sample rate
}

void loop() {
  digitalWrite(COL_PIN, LOW);           // drive column LOW
  if (digitalRead(ROW_PIN) == LOW) {    // key pressed
    Serial.println("Key pressed! Playing WAV...");
    audio.stop();                        // stop any previous playback
    audio.begin(audioFile, 32000);       // start WAV at 32kHz
    delay(200);                           // debounce
  }
  digitalWrite(COL_PIN, HIGH);          // release column
  delay(10);
}