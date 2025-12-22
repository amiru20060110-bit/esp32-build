#include <driver/i2s.h>
#include "synas4.h"  // A#4
#include "synds4.h"  // D#4
#include "sync5.h"   // C5

// ===== I2S =====
#define I2S_BCLK 1
#define I2S_LRC  2
#define I2S_DOUT 3

#define SAMPLE_RATE 44100
#define VOLUME 12000

// ===== MATRIX =====
const int ROWS[] = {15, 16, 17, 18};
const int COLS[] = {21, 22, 23, 24};
#define NUM_ROWS 4
#define NUM_COLS 4  // 16 possible, we use 13 keys

// WAV data pointers
struct Note {
  const byte* data;
  size_t len;
};
Note notes[13];

// Pitch shift multipliers for other keys (C4→C5)
const float pitches[13] = {
  0.5f,   // C4 (C5 * 0.5)
  0.53f,  // C#4
  0.56f,  // D4
  0.59f,  // D#4 -> already have synds4
  0.63f,  // E4
  0.667f, // F4
  0.707f, // F#4
  0.75f,  // G4
  0.793f, // G#4
  0.84f,  // A4
  0.89f,  // A#4 -> already have synas4
  0.943f, // B4
  1.0f    // C5 -> already have sync5
};

float phase = 0;
int activeKey = -1;
size_t wavPos = 0;

// ===== I2S =====
void i2sInit() {
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
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };
  i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pins);
}

void setup() {
  // Matrix setup
  for (int r = 0; r < NUM_ROWS; r++) {
    pinMode(ROWS[r], OUTPUT);
    digitalWrite(ROWS[r], HIGH);
  }
  for (int c = 0; c < NUM_COLS; c++) pinMode(COLS[c], INPUT_PULLDOWN);

  // Notes setup
  notes[0]  = {sync5, sizeof(sync5)};   // C5
  notes[1]  = {sync5, sizeof(sync5)};   // C#4 (pitch shift)
  notes[2]  = {sync5, sizeof(sync5)};   // D4
  notes[3]  = {synds4, sizeof(synds4)}; // D#4
  notes[4]  = {sync5, sizeof(sync5)};   // E4
  notes[5]  = {sync5, sizeof(sync5)};   // F4
  notes[6]  = {sync5, sizeof(sync5)};   // F#4
  notes[7]  = {sync5, sizeof(sync5)};   // G4
  notes[8]  = {sync5, sizeof(sync5)};   // G#4
  notes[9]  = {sync5, sizeof(sync5)};   // A4
  notes[10] = {synas4, sizeof(synas4)}; // A#4
  notes[11] = {sync5, sizeof(sync5)};   // B4
  notes[12] = {sync5, sizeof(sync5)};   // C5
  i2sInit();
}

void loop() {
  size_t bw;

  // Scan matrix
  activeKey = -1;
  for (int r = 0; r < NUM_ROWS; r++) {
    digitalWrite(ROWS[r], LOW);
    delayMicroseconds(3);
    for (int c = 0; c < NUM_COLS; c++) {
      int idx = r * NUM_COLS + c;
      if (idx >= 13) break; // only 13 keys
      if (digitalRead(COLS[c]) == LOW) activeKey = idx;
    }
    digitalWrite(ROWS[r], HIGH);
  }

  // Playback
  int16_t sample = 0;
  if (activeKey >= 0) {
    const Note &n = notes[activeKey];
    float pitch = pitches[activeKey];
    wavPos += pitch;
    if (wavPos >= n.len) wavPos -= n.len;
    sample = ((int16_t)pgm_read_byte(&n.data[(int)wavPos]) - 128) << 8;
  }
  i2s_write(I2S_NUM_0, &sample, sizeof(sample), &bw, portMAX_DELAY);
}