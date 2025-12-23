#include <Arduino.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "driver/i2s.h"

// --- Hardware Pins ---
#define SD_MOSI 6
#define SD_MISO 5
#define SD_CLK  7
#define SD_CS   4

#define I2S_BCLK 1
#define I2S_LRC  2
#define I2S_DOUT 3

// Matrix Config: 6 Columns x 5 Rows
// Swapped 48 for 38 to avoid the onboard RGB LED
const int colPins[6] = {33, 34, 40, 41, 42, 38}; 
const int rowPins[5] = {15, 16, 17, 18, 47}; 

const char* soundFiles[5][6] = {
  {"/25.wav", "/26.wav", "/27.wav", "/28.wav", "/29.wav", "/30.wav"},
  {"/31.wav", "/32.wav", "/33.wav", "/34.wav", "/35.wav", "/36.wav"},
  {"/37.wav", "/38.wav", "/39.wav", "/40.wav", "/17.wav", "/18.wav"},
  {"/19.wav", "/20.wav", "/21.wav", "/22.wav", "/23.wav", "/24.wav"},
  {"/41.wav", "/42.wav", "/43.wav", "/44.wav", "/45.wav", "/46.wav"}
};

// --- Pro-Audio Low Latency Settings ---
#define MAX_VOICES 4
#define SAMPLE_RATE 32000
#define I2S_NUM I2S_NUM_0
#define BUF_SIZE 256        // Tiny buffer for instant response
#define DMA_COUNT 4         // Minimal buffering
#define SCAN_DELAY_US 10    // Fast matrix settling time

struct Voice {
  File file;
  int row = -1;
  int col = -1;
  bool active = false;
};

Voice voices[MAX_VOICES];
bool keyStates[5][6] = {false}; 

void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = DMA_COUNT,
    .dma_buf_len = BUF_SIZE,
    .use_apll = false,
    .tx_desc_auto_clear = true 
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
  // Serial disabled for speed, enable only for debugging
  // Serial.begin(115200);

  for (int i = 0; i < 6; i++) {
    pinMode(colPins[i], OUTPUT);
    digitalWrite(colPins[i], LOW);
  }
  for (int i = 0; i < 5; i++) {
    pinMode(rowPins[i], INPUT_PULLDOWN);
  }

  // SPI overclocked to 40MHz to handle multi-voice reading
  SPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, SPI, 40000000)) { 
    while(1); 
  }

  setupI2S();
}

void loop() {
  // 1. Matrix Scan
  for (int c = 0; c < 6; c++) {
    digitalWrite(colPins[c], HIGH);
    delayMicroseconds(SCAN_DELAY_US); 
    
    for (int r = 0; r < 5; r++) {
      bool pressed = (digitalRead(rowPins[r]) == HIGH);
      
      if (pressed && !keyStates[r][c]) {
        for (int i = 0; i < MAX_VOICES; i++) {
          if (!voices[i].active) {
            voices[i].file = SD.open(soundFiles[r][c]);
            if (voices[i].file) {
              voices[i].file.seek(44); // Jump straight to audio data
              voices[i].active = true;
              voices[i].row = r;
              voices[i].col = c;
            }
            break; 
          }
        }
      } 
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

  // 2. High-Speed Audio Mixing
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
          int32_t mixed = (int32_t)mixBuf[j] + (int32_t)tempBuf[j];
          // Optimization: Hard Clipping
          if (mixed > 32767) mixed = 32767;
          else if (mixed < -32768) mixed = -32768;
          mixBuf[j] = (int16_t)mixed;
        }
      } else {
        stopVoice(i);
      }
    }
  }

  // 3. I2S Write (Zero timeout = No waiting)
  if (anyActive) {
    size_t written;
    i2s_write(I2S_NUM, mixBuf, sizeof(mixBuf), &written, 0);
  }
}
