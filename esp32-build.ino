#include <Arduino.h>
#include <driver/i2s.h>
#include "synas4.h"   // AS4 WAV
#include "synds4.h"   // DS4 WAV
#include "sync5.h"    // C5 WAV

// ================= I2S PINS =================
#define I2S_BCLK   1
#define I2S_LRC    2
#define I2S_DOUT   3

// ================= KEYS =====================
#define ROW_AS4  15
#define ROW_DS4  16
#define ROW_C5   17

#define COL_KEY  21  // active LOW input for all keys

// ================= AUDIO ====================
#define SAMPLE_RATE 44100
#define WAV_HEADER_SIZE 44
#define BUFFER_SAMPLES 256   // small RAM usage

int wav_index = WAV_HEADER_SIZE;
bool playing = false;
const byte* currentWav = nullptr;
size_t currentWavSize = 0;

// ================= SETUP ====================
void setup() {
  // ---- Matrix (keys) ----
  pinMode(ROW_AS4, OUTPUT); digitalWrite(ROW_AS4, HIGH);
  pinMode(ROW_DS4, OUTPUT); digitalWrite(ROW_DS4, HIGH);
  pinMode(ROW_C5,  OUTPUT); digitalWrite(ROW_C5, HIGH);

  pinMode(COL_KEY, INPUT_PULLUP);

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
  // ---- Scan keys ----
  bool keyPressed = false;

  // AS4 key
  digitalWrite(ROW_AS4, LOW);
  delayMicroseconds(3);
  if (digitalRead(COL_KEY) == LOW) {
    keyPressed = true;
    currentWav = synas4;
    currentWavSize = sizeof(synas4);
  }
  digitalWrite(ROW_AS4, HIGH);

  // DS4 key
  digitalWrite(ROW_DS4, LOW);
  delayMicroseconds(3);
  if (!keyPressed && digitalRead(COL_KEY) == LOW) {
    keyPressed = true;
    currentWav = synds4;
    currentWavSize = sizeof(synds4);
  }
  digitalWrite(ROW_DS4, HIGH);

  // C5 key
  digitalWrite(ROW_C5, LOW);
  delayMicroseconds(3);
  if (!keyPressed && digitalRead(COL_KEY) == LOW) {
    keyPressed = true;
    currentWav = sync5;
    currentWavSize = sizeof(sync5);
  }
  digitalWrite(ROW_C5, HIGH);

  // ---- Start playing if pressed ----
  if (keyPressed && !playing) {
    wav_index = WAV_HEADER_SIZE;  // skip WAV header
    playing = true;
  }

  if (!keyPressed) {
    playing = false;
  }

  // ---- Play WAV ----
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