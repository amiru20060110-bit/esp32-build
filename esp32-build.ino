#include <Arduino.h>
#include <driver/i2s.h>

#include "ce4.h"
#include "cs4.h"
#include "d4.h"
#include "ds4.h"
#include "e4.h"

// ===== I2S PINS =====
#define I2S_BCLK 1
#define I2S_LRC  2
#define I2S_DOUT 3

// ===== MATRIX =====
#define COL0 40
#define COL1 41

#define ROW0 15
#define ROW1 16
#define ROW2 17

// ===== AUDIO =====
#define SAMPLE_RATE 32000
#define WAV_HEADER_SIZE 44
#define BUFFER_SAMPLES 256

const uint8_t* current_wav = nullptr;
uint32_t wav_size = 0;
uint32_t wav_pos = WAV_HEADER_SIZE;
bool playing = false;

// ===== SETUP =====
void setup() {
  // Columns
  pinMode(COL0, INPUT_PULLUP);
  pinMode(COL1, INPUT_PULLUP);

  // Rows
  pinMode(ROW0, OUTPUT);
  pinMode(ROW1, OUTPUT);
  pinMode(ROW2, OUTPUT);

  digitalWrite(ROW0, HIGH);
  digitalWrite(ROW1, HIGH);
  digitalWrite(ROW2, HIGH);

  // I2S config
  i2s_config_t cfg = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 256,
    .use_apll = true,
    .tx_desc_auto_clear = true
  };

  i2s_pin_config_t pins = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pins);
}

// ===== SCAN ONE ROW =====
void scanRow(int rowPin,
             const uint8_t* wav0, uint32_t size0,
             const uint8_t* wav1, uint32_t size1) {

  digitalWrite(rowPin, LOW);
  delayMicroseconds(3);

  if (digitalRead(COL0) == LOW) {
    current_wav = wav0;
    wav_size = size0;
  }
  if (digitalRead(COL1) == LOW) {
    current_wav = wav1;
    wav_size = size1;
  }

  digitalWrite(rowPin, HIGH);
}

// ===== LOOP =====
void loop() {
  current_wav = nullptr;

  scanRow(ROW0, ce4, sizeof(ce4), cs4, sizeof(cs4));
  scanRow(ROW1, d4,  sizeof(d4),  ds4, sizeof(ds4));
  scanRow(ROW2, e4,  sizeof(e4),  nullptr, 0);

  // No key pressed
  if (!current_wav) {
    playing = false;
    return;
  }

  // Start once per key press
  if (!playing) {
    wav_pos = WAV_HEADER_SIZE;
    playing = true;
  }

  // Stop when sample ends (no looping)
  if (wav_pos >= wav_size - 2) {
    playing = false;
    return;
  }

  int16_t buffer[BUFFER_SAMPLES];
  int samples = 0;

  while (samples < BUFFER_SAMPLES && wav_pos < wav_size - 2) {
    buffer[samples++] =
      (int16_t)(current_wav[wav_pos] |
               (current_wav[wav_pos + 1] << 8));
    wav_pos += 2;
  }

  size_t bw;
  i2s_write(I2S_NUM_0, buffer, samples * 2, &bw, portMAX_DELAY);
}