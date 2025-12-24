#include <Arduino.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "driver/i2s.h"

#define SD_MOSI 6
#define SD_MISO 5
#define SD_CLK  7
#define SD_CS   4

#define I2S_BCLK 1
#define I2S_LRC  44
#define I2S_DOUT 3

// Final Matrix Config (8 Columns x 8 Rows = 64 keys)
// Columns: Added GPIO 47
const int colPins[8] = {40, 41, 42, 33, 34, 35, 39, 2}; 
// Rows: Includes 45 from the previous step
const int rowPins[8] = {15, 16, 17, 18, 38, 36, 37, 45}; 

// Mapping physical keyboard PA0-PA7 (Rows) and PB0-PB7 (Cols) to specific files
const char* soundFiles[8][8] = {
  // PA0: Files 16 to 23 (Starting at C2)
  {"/16.wav", "/17.wav", "/18.wav", "/19.wav", "/20.wav", "/21.wav", "/22.wav", "/23.wav"},
  // PA1: Files 24 to 31
  {"/24.wav", "/25.wav", "/26.wav", "/27.wav", "/28.wav", "/29.wav", "/30.wav", "/31.wav"},
  // PA2: Files 32 to 39
  {"/32.wav", "/33.wav", "/34.wav", "/35.wav", "/36.wav", "/37.wav", "/38.wav", "/39.wav"},
  // PA3: Files starting with ce4.wav (C4) up to 47.wav
  {"/ce4.wav", "/41.wav", "/42.wav", "/43.wav", "/44.wav", "/45.wav", "/46.wav", "/47.wav"},
  // PA4: Files 48 to 55
  {"/48.wav", "/49.wav", "/50.wav", "/51.wav", "/52.wav", "/53.wav", "/54.wav", "/55.wav"},
  // PA5: Files 56 to 63
  {"/56.wav", "/57.wav", "/58.wav", "/59.wav", "/60.wav", "/61.wav", "/62.wav", "/63.wav"},
  // PA6: Files 64 to 71
  {"/64.wav", "/65.wav", "/66.wav", "/67.wav", "/68.wav", "/69.wav", "/70.wav", "/71.wav"},
  // PA7: Files 72 to 76 (76.wav is the 61st key)
  {"/72.wav", "/73.wav", "/74.wav", "/75.wav", "/76.wav", "none", "none", "none"}
};


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
bool keyStates[8][8] = {false}; 

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
  delay(2000); 

  for (int i = 0; i < 8; i++) {
    pinMode(colPins[i], OUTPUT);
    digitalWrite(colPins[i], LOW);
    pinMode(rowPins[i], INPUT_PULLDOWN);
  }

  SPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS, SPI, 20000000)) { 
    Serial.println("SD Error!"); 
    while(1); 
  }

  setupI2S();
  Serial.println("System Online: 64-Key / 4-Voice Polyphony Ready");
}

void loop() {
  for (int c = 0; c < 8; c++) {
    digitalWrite(colPins[c], HIGH);
    delayMicroseconds(30); 
    
    for (int r = 0; r < 8; r++) {
      bool pressed = (digitalRead(rowPins[r]) == HIGH);
      
      if (pressed && !keyStates[r][c]) {
        for (int i = 0; i < MAX_VOICES; i++) {
          if (!voices[i].active) {
            voices[i].file = SD.open(soundFiles[r][c]);
            if (voices[i].file) {
              voices[i].file.seek(44); 
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
          if (mixed > 32767) mixed = 32767;
          else if (mixed < -32768) mixed = -32768;
          mixBuf[j] = (int16_t)mixed;
        }
      } else {
        stopVoice(i);
      }
    }
  }

  if (anyActive) {
    size_t written;
    i2s_write(I2S_NUM, mixBuf, sizeof(mixBuf), &written, portMAX_DELAY);
  } else {
    i2s_zero_dma_buffer(I2S_NUM);
  }
}
