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

const int colPins[8] = {40, 41, 42, 33, 34, 35, 39, 2}; 
const int rowPins[8] = {15, 16, 17, 18, 38, 36, 37, 45}; 

const char* soundFiles[8][8] = {
  {"/16.wav", "/17.wav", "/18.wav", "/19.wav", "/20.wav", "/21.wav", "/22.wav", "/23.wav"},
  {"/24.wav", "/25.wav", "/26.wav", "/27.wav", "/28.wav", "/29.wav", "/30.wav", "/31.wav"},
  {"/32.wav", "/33.wav", "/34.wav", "/35.wav", "/36.wav", "/37.wav", "/38.wav", "/39.wav"},
  {"/ce4.wav", "/41.wav", "/42.wav", "/43.wav", "/44.wav", "/45.wav", "/46.wav", "/47.wav"},
  {"/48.wav", "/49.wav", "/50.wav", "/51.wav", "/52.wav", "/53.wav", "/54.wav", "/55.wav"},
  {"/56.wav", "/57.wav", "/58.wav", "/59.wav", "/60.wav", "/61.wav", "/62.wav", "/63.wav"},
  {"/64.wav", "/65.wav", "/66.wav", "/67.wav", "/68.wav", "/69.wav", "/70.wav", "/71.wav"},
  {"/72.wav", "/73.wav", "/74.wav", "/75.wav", "/76.wav", "SWITCH", "none", "none"}
};

#define MAX_VOICES 4
#define SAMPLE_RATE 32000
#define BUF_SIZE 512 
#define FADE_SAMPLES 150 // Slightly longer fade-in for vibraphone
#define GLOBAL_GAIN 0.55f // Master volume to prevent clipping

struct Voice {
  File file;
  int row = -1, col = -1;
  bool active = false;
  uint32_t samplesPlayed = 0;
  int16_t lastSample = 0;
  float noteGain = 1.0f; // Individual gain per note
};

Voice voices[MAX_VOICES];
volatile bool sharedKeys[8][8] = {false}; 
bool keyStates[8][8] = {false}; 
int currentBank = 0; 

void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8, 
    .dma_buf_len = 256,
    .use_apll = false
  };
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK, .ws_io_num = I2S_LRC, .data_out_num = I2S_DOUT, .data_in_num = I2S_PIN_NO_CHANGE
  };
  i2s_set_pin(I2S_NUM_0, &pin_config);
}

void scanTask(void * pvParameters) {
  for(;;) {
    for (int c = 0; c < 8; c++) {
      digitalWrite(colPins[c], HIGH);
      delayMicroseconds(10); 
      for (int r = 0; r < 8; r++) {
        sharedKeys[r][c] = (digitalRead(rowPins[r]) == HIGH);
      }
      digitalWrite(colPins[c], LOW);
    }
    vTaskDelay(1); 
  }
}

void setup() {
  for (int i = 0; i < 8; i++) {
    pinMode(colPins[i], OUTPUT); digitalWrite(colPins[i], LOW);
    pinMode(rowPins[i], INPUT_PULLDOWN);
  }
  SPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);
  SD.begin(SD_CS, SPI, 40000000); 
  setupI2S();
  xTaskCreatePinnedToCore(scanTask, "Scanner", 4096, NULL, 1, NULL, 0);
}

void loop() {
  for (int c = 0; c < 8; c++) {
    for (int r = 0; r < 8; r++) {
      bool pressed = sharedKeys[r][c];
      if (pressed && !keyStates[r][c]) {
        if (r == 7 && c == 5) { currentBank = (currentBank == 0) ? 1 : 0; } 
        else if (strcmp(soundFiles[r][c], "none") != 0 && strcmp(soundFiles[r][c], "SWITCH") != 0) {
          for (int i = 0; i < MAX_VOICES; i++) {
            if (!voices[i].active) {
              char fullPath[32];
              snprintf(fullPath, sizeof(fullPath), "/%c%s", (currentBank == 0 ? 'P' : 'X'), soundFiles[r][c] + 1);
              voices[i].file = SD.open(fullPath);
              if (voices[i].file) {
                voices[i].file.seek(44); 
                voices[i].active = true;
                voices[i].samplesPlayed = 0;
                voices[i].lastSample = 0;
                voices[i].row = r; voices[i].col = c;
                
                // --- NEW: Damping logic for high notes ---
                // Low notes (row 0-2) stay loud. High notes (row 6-7) get quieter.
                voices[i].noteGain = 1.0f - (r * 0.08f); 
                if (currentBank == 1) voices[i].noteGain *= 0.8f; // Extra damping for Vibraphone
              }
              break; 
            }
          }
        }
      } 
      else if (!pressed && keyStates[r][c]) {
        for (int i = 0; i < MAX_VOICES; i++) {
          if (voices[i].active && voices[i].row == r && voices[i].col == c) {
            voices[i].file.close(); voices[i].active = false;
          }
        }
      }
      keyStates[r][c] = pressed;
    }
  }

  int16_t mixBuf[BUF_SIZE] = {0};
  bool anyActive = false;
  
  for (int i = 0; i < MAX_VOICES; i++) {
    if (voices[i].active && voices[i].file.available()) {
      anyActive = true;
      int16_t tempBuf[BUF_SIZE];
      size_t bytesRead = voices[i].file.read((uint8_t*)tempBuf, BUF_SIZE * 2);
      
      for (int j = 0; j < (int)(bytesRead / 2); j++) {
        float vol = (voices[i].samplesPlayed < FADE_SAMPLES) ? (float)voices[i].samplesPlayed / FADE_SAMPLES : 1.0;
        
        int16_t currentSample = tempBuf[j];
        // Linear Interpolation for smoothness
        int16_t smoothedSample = (currentSample + voices[i].lastSample) / 2;
        voices[i].lastSample = currentSample;

        // Apply Global Gain + Note-Specific Scaling + Soft Fade
        int32_t sample = (int32_t)((float)smoothedSample * vol * GLOBAL_GAIN * voices[i].noteGain);
        int32_t mixed = (int32_t)mixBuf[j] + sample;
        
        mixBuf[j] = (int16_t)constrain(mixed, -32768, 32767);
        voices[i].samplesPlayed++;
      }
    } else if (voices[i].active) {
      voices[i].file.close(); voices[i].active = false;
    }
  }

  if (anyActive) {
    size_t written;
    i2s_write(I2S_NUM_0, mixBuf, sizeof(mixBuf), &written, portMAX_DELAY);
  } else {
    i2s_zero_dma_buffer(I2S_NUM_0);
  }
}
