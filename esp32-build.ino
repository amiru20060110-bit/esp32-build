#include <Arduino.h>
#include <driver/i2s.h>
#include "synas4.h"   // your WAV array

// ================= I2S PINS =================
#define I2S_BCLK   1
#define I2S_LRC    2
#define I2S_DOUT   3

// ================= MATRIX (1 key) ===========
#define ROW0  15
#define COL0  40   // active LOW

// ================= AUDIO ====================
#define SAMPLE_RATE 44100
#define WAV_HEADER_SIZE 44
#define BUFFER_SAMPLES 256   // small RAM usage

int wav_index = WAV_HEADER_SIZE;
bool playing = false;

// ================= SETUP ====================
void setup() {
  // ---- Matrix ----
  pinMode(ROW0, OUTPUT);
  digitalWrite(ROW0, HIGH);

  pinMode(COL0, INPUT_PULLUP);

  // ---- I2S ----
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
}

// ================= LOOP =====================
void loop() {
  // ---- Scan key ----
  digitalWrite(ROW0, LOW);
  delayMicroseconds(3);
  bool keyPressed = (digitalRead(COL0) == LOW);
  digitalWrite(ROW0, HIGH);

  if (keyPressed && !playing) {
    wav_index = WAV_HEADER_SIZE;
    playing = true;
  }

  if (!keyPressed) {
    playing = false;
  }

  if (!playing) return;

  // ---- Audio stream ----
  int16_t buffer[BUFFER_SAMPLES];
  int samples = 0;

  while (samples < BUFFER_SAMPLES && wav_index < sizeof(synas4)) {
    buffer[samples++] =
      (int16_t)(synas4[wav_index] | (synas4[wav_index + 1] << 8));
    wav_index += 2;
  }

  if (samples == 0) {
    playing = false;
    return;
  }

  size_t bytesWritten;
  i2s_write(I2S_NUM_0, buffer, samples * 2, &bytesWritten, portMAX_DELAY);
}