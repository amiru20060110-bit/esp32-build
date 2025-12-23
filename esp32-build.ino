#include <Arduino.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "driver/i2s.h"

// SD Card Pins
#define SD_MOSI 6
#define SD_MISO 5
#define SD_CLK  7
#define SD_CS   4

// I2S Pins
#define I2S_BCLK 1
#define I2S_LRC  2
#define I2S_DOUT 3

// Matrix Pins (3x3)
const int colPins[3] = {40, 41, 42};
const int rowPins[3] = {15, 16, 17};

// File Mapping (Row x Column)
const char* soundFiles[3][3] = {
  {"/16.wav",  "/17.wav",  "/18.wav"},
  {"/35.wav",  "/ce4.wav", "/60.wav"},
  {"/61.wav",  "/62.wav",  "/63.wav"}
};

// Audio Config
#define SAMPLE_RATE 32000
#define I2S_NUM     I2S_NUM_0

File audioFile;
bool isPlaying = false;
int activeKeyRow = -1;
int activeKeyCol = -1;
bool keyWasPressedLastCycle = false;

void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 4,
    .dma_buf_len = 256,
    .use_apll = false
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM, &pin_config);
}

void setup() {
  Serial.begin(115200);

  // Matrix Setup
  for (int i = 0; i < 3; i++) {
    pinMode(colPins[i], OUTPUT);
    digitalWrite(colPins[i], LOW);
    pinMode(rowPins[i], INPUT_PULLDOWN);
  }

  // SD Setup
  SPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS)) {
    Serial.println("SD Mount Failed");
    while (1);
  }

  setupI2S();
  Serial.println("3x3 Matrix Audio System Online");
}

void loop() {
  int currentPressedRow = -1;
  int currentPressedCol = -1;
  bool anyKeyPressed = false;

  // --- Scan 3x3 Matrix ---
  for (int c = 0; c < 3; c++) {
    digitalWrite(colPins[c], HIGH); // Pulse Column
    for (int r = 0; r < 3; r++) {
      if (digitalRead(rowPins[r]) == HIGH) { // Check Row
        currentPressedRow = r;
        currentPressedCol = c;
        anyKeyPressed = true;
        break;
      }
    }
    digitalWrite(colPins[c], LOW);
    if (anyKeyPressed) break; 
  }

  if (anyKeyPressed) {
    // Only Trigger if it's a fresh press (One-Time Play Logic)
    if (!keyWasPressedLastCycle) {
      if (isPlaying) {
        audioFile.close();
        i2s_zero_dma_buffer(I2S_NUM);
      }
      
      activeKeyRow = currentPressedRow;
      activeKeyCol = currentPressedCol;
      audioFile = SD.open(soundFiles[activeKeyRow][activeKeyCol]);
      
      if (audioFile) {
        audioFile.seek(44); // Skip header
        isPlaying = true;
        Serial.printf("Playing: %s\n", soundFiles[activeKeyRow][activeKeyCol]);
      }
    }

    // Streaming loop
    if (isPlaying && audioFile.available()) {
      static uint8_t buf[256];
      size_t bytesRead = audioFile.read(buf, sizeof(buf));
      size_t bytesWritten;
      i2s_write(I2S_NUM, buf, bytesRead, &bytesWritten, portMAX_DELAY);
    } 
    else if (isPlaying && !audioFile.available()) {
      // Auto-stop at end of file
      isPlaying = false;
      audioFile.close();
      i2s_zero_dma_buffer(I2S_NUM);
    }
  } 
  else {
    // IMMEDIATE STOP on key release
    if (isPlaying) {
      isPlaying = false;
      audioFile.close();
      i2s_zero_dma_buffer(I2S_NUM);
      Serial.println("Released: Silence.");
    }
  }

  keyWasPressedLastCycle = anyKeyPressed;
}
