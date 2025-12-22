#include <Arduino.h>
#include <driver/i2s.h>
#include "synas4.h"   // AS4 WAV
#include "synds4.h"   // DS4 WAV
#include "sync5.h"    // C5 WAV

// ================= I2S PINS =================
#define I2S_BCLK   1
#define I2S_LRC    2
#define I2S_DOUT   3

// ================= MATRIX PINS =================
const uint8_t ROWS[] = {15, 16, 17};   // AS4, DS4, C5
const uint8_t COLS[] = {40};           // Shared column pin
#define NUM_ROWS 3
#define NUM_COLS 1

// ================= AUDIO ====================
#define SAMPLE_RATE 44100
#define WAV_HEADER_SIZE 44
#define BUFFER_SAMPLES 256

int wav_index = WAV_HEADER_SIZE;
bool playing = false;
const byte* currentWav = nullptr;
size_t currentWavSize = 0;

// ================= SETUP ====================
void setup() {
  Serial.begin(115200);

  // Matrix setup
  for (int r = 0; r < NUM_ROWS; r++) {
    pinMode(ROWS[r], OUTPUT);
    digitalWrite(ROWS[r], HIGH); // idle HIGH
  }
  for (int c = 0; c < NUM_COLS; c++) {
    pinMode(COLS[c], INPUT_PULLUP);
  }

  // I2S setup
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .dma_buf_count = 8,
    .dma_buf_len = 256,
    .use_apll = true,
    .tx_desc_auto_clear = true
  };

  i2s_pin_config_t pins = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num  = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num  = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pins);

  Serial.println("Keyboard ready with row->column scanning");
}

// ================= LOOP =====================
void loop() {
  bool keyPressed = false;

  // Row -> column scanning
  for (int r = 0; r < NUM_ROWS; r++) {
    digitalWrite(ROWS[r], LOW);
    delayMicroseconds(3);  // small delay for stabilization

    for (int c = 0; c < NUM_COLS; c++) {
      if (digitalRead(COLS[c]) == LOW) { // active LOW = pressed
        keyPressed = true;

        // Map row to WAV
        switch (r) {
          case 0: currentWav = synas4; currentWavSize = sizeof(synas4); break; // AS4
          case 1: currentWav = synds4; currentWavSize = sizeof(synds4); break; // DS4
          case 2: currentWav = sync5;  currentWavSize = sizeof(sync5);  break; // C5
        }
      }
    }
    digitalWrite(ROWS[r], HIGH);
  }

  // Start playing if pressed
  if (keyPressed && !playing) {
    wav_index = WAV_HEADER_SIZE;  // skip WAV header
    playing = true;
  }

  if (!keyPressed) {
    playing = false;
  }

  // Play WAV
  if (!playing) return;

  int16_t buffer[BUFFER_SAMPLES];
  int samples = 0;

  while (samples < BUFFER_SAMPLES && wav_index < currentWavSize) {
    buffer[samples++] =
      (int16_t)(currentWav[wav_index] | (currentWav[wav_index + 1] << 8));
    wav_index += 2;
  }

  if (samples == 0) {
    playing = false;
    return;
  }

  size_t bytesWritten;
  i2s_write(I2S_NUM_0, buffer, samples * 2, &bytesWritten, portMAX_DELAY);
}