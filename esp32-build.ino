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

// Matrix Pins
const int colPins[2] = {40, 41};
const int rowPins[2] = {15, 16};

// File Mapping
const char* soundFiles[2][2] = {
  {"/ce4.wav", "/16.wav"},
  {"/17.wav",  "/18.wav"}
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
  for (int i = 0; i < 2; i++) {
    pinMode(colPins[i], OUTPUT);
    digitalWrite(colPins[i], LOW);
    pinMode(rowPins[i], INPUT_PULLDOWN);
  }

  // SD Setup
  SPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS)) {
    Serial.println("SD Error");
    while (1);
  }

  setupI2S();
  Serial.println("2x2 Matrix Ready.");
}

void loop() {
  int currentPressedRow = -1;
  int currentPressedCol = -1;
  bool anyKeyPressed = false;

  // --- Scan 2x2 Matrix ---
  for (int c = 0; c < 2; c++) {
    digitalWrite(colPins[c], HIGH);
    for (int r = 0; r < 2; r++) {
      if (digitalRead(rowPins[r]) == HIGH) {
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
    // If it's a new press or a different key than before
    if (!keyWasPressedLastCycle || (currentPressedRow != activeKeyRow || currentPressedCol != activeKeyCol)) {
      if (isPlaying) {
        audioFile.close();
        i2s_zero_dma_buffer(I2S_NUM);
      }
      
      activeKeyRow = currentPressedRow;
      activeKeyCol = currentPressedCol;
      audioFile = SD.open(soundFiles[activeKeyRow][activeKeyCol]);
      
      if (audioFile) {
        audioFile.seek(44); // Skip WAV header
        isPlaying = true;
        Serial.printf("Playing: %s\n", soundFiles[activeKeyRow][activeKeyCol]);
      }
    }

    // Stream Audio
    if (isPlaying && audioFile.available()) {
      static uint8_t buf[256];
      size_t bytesRead = audioFile.read(buf, sizeof(buf));
      size_t bytesWritten;
      i2s_write(I2S_NUM, buf, bytesRead, &bytesWritten, portMAX_DELAY);
    } 
    else if (isPlaying && !audioFile.available()) {
      // Finished playing (one-time play logic)
      isPlaying = false;
      audioFile.close();
      i2s_zero_dma_buffer(I2S_NUM);
    }
  } 
  else {
    // IMMEDIATE STOP on release
    if (isPlaying) {
      isPlaying = false;
      audioFile.close();
      i2s_zero_dma_buffer(I2S_NUM);
      Serial.println("Key Released - Stopped.");
    }
    activeKeyRow = -1;
    activeKeyCol = -1;
  }

  keyWasPressedLastCycle = anyKeyPressed;
}
