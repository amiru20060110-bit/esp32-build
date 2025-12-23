#include <Arduino.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "driver/i2s.h"

// SD Card Pins (SPI)
#define SD_MOSI 6
#define SD_MISO 5
#define SD_CLK  7
#define SD_CS   4

// I2S Pins
#define I2S_BCLK 1
#define I2S_LRC  2
#define I2S_DOUT 3

// Matrix Config (4x4)
const int colPins[4] = {40, 41, 42, 22};
const int rowPins[4] = {15, 16, 17, 21};

// File Mapping (Swapped 40.wav for ce4.wav)
const char* soundFiles[4][4] = {
  {"/16.wav", "/17.wav", "/18.wav", "/19.wav"},
  {"/20.wav", "/21.wav", "/ce4.wav", "/41.wav"},
  {"/42.wav", "/43.wav", "/44.wav", "/45.wav"},
  {"/46.wav", "/47.wav", "/48.wav", "/49.wav"}
};

// Polyphony & Audio Config
#define MAX_VOICES 4
#define SAMPLE_RATE 32000
#define I2S_NUM I2S_NUM_0
#define BUF_SIZE 512 

struct Voice {
  File file;
  int row = -1;
  int col = -1;
  bool active = false;
};

Voice voices[MAX_VOICES];
bool keyStates[4][4] = {false}; 

void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = BUF_SIZE,
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

void stopVoice(int i) {
  if (voices[i].active) {
    voices[i].file.close();
    voices[i].active = false;
    voices[i].row = -1;
    voices[i].col = -1;
  }
}

void setup() {
  Serial.begin(115200);
  
  // Matrix Pin Init
  for (int i = 0; i < 4; i++) {
    pinMode(colPins[i], OUTPUT);
    digitalWrite(colPins[i], LOW);
    pinMode(rowPins[i], INPUT_PULLDOWN);
  }

  // High-speed SPI for polyphony
  SPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, SPI, 20000000)) { 
    Serial.println("SD initialization failed!"); 
    while(1); 
  }

  setupI2S();
  Serial.println("System Ready: 4-Voice 4x4 Sampler");
}

void loop() {
  // 1. Scan Matrix (Column -> Row)
  for (int c = 0; c < 4; c++) {
    digitalWrite(colPins[c], HIGH);
    for (int r = 0; r < 4; r++) {
      bool pressed = (digitalRead(rowPins[r]) == HIGH);
      
      // Trigger on Press
      if (pressed && !keyStates[r][c]) {
        for (int i = 0; i < MAX_VOICES; i++) {
          if (!voices[i].active) {
            voices[i].file = SD.open(soundFiles[r][c]);
            if (voices[i].file) {
              voices[i].file.seek(44); // Skip header
              voices[i].active = true;
              voices[i].row = r;
              voices[i].col = c;
              Serial.printf("Playing: %s on Voice %d\n", soundFiles[r][c], i);
            }
            break; 
          }
        }
      } 
      // Stop on Release
      else if (!pressed && keyStates[r][c]) {
        for (int i = 0; i < MAX_VOICES; i++) {
          if (voices[i].active && voices[i].row == r && voices[i].col == c) {
            stopVoice(i);
          }
        }
      }
      keyStates[r][c] = pressed;
    }
    digitalWrite(colPins[c], LOW);
  }

  // 2. Audio Mixing Logic
  int16_t mixBuf[BUF_SIZE];
  memset(mixBuf, 0, sizeof(mixBuf));
  bool anyActive = false;

  for (int i = 0; i < MAX_VOICES; i++) {
    if (voices[i].active) {
      if (voices[i].file.available()) {
        anyActive = true;
        int16_t tempBuf[BUF_SIZE];
        size_t bytesRead = voices[i].file.read((uint8_t*)tempBuf, BUF_SIZE * 2);
        int samplesRead = bytesRead / 2;
        
        for (int j = 0; j < samplesRead; j++) {
          // Mix with 32-bit math to prevent wrap-around distortion
          int32_t mixed = (int32_t)mixBuf[j] + (int32_t)tempBuf[j];
          
          // Hard Clamping (Limiting)
          if (mixed > 32767) mixed = 32767;
          else if (mixed < -32768) mixed = -32768;
          
          mixBuf[j] = (int16_t)mixed;
        }
      } else {
        stopVoice(i); // Auto-stop at end of file
      }
    }
  }

  if (anyActive) {
    size_t written;
    i2s_write(I2S_NUM, mixBuf, sizeof(mixBuf), &written, portMAX_DELAY);
  } else {
    // If no voices are active, ensure the I2S hardware is silent
    i2s_zero_dma_buffer(I2S_NUM);
  }
}
