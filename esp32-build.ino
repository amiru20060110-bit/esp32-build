#include <Arduino.h>
#include <driver/i2s.h>

#include "synas4.h"   // AS4
#include "synds4.h"   // DS4
#include "sync5.h"    // C5

// ================= I2S =================
#define I2S_BCLK   1
#define I2S_LRC    2
#define I2S_DOUT   3

// ================= MATRIX =================
// Column -> Row diode orientation
// Rows active-LOW, Columns INPUT_PULLUP

#define ROW0  15

#define COL0  40   // AS4
#define COL1  41   // DS4
#define COL2  42   // C5

// ================= AUDIO =================
#define SAMPLE_RATE     44100
#define WAV_HEADER_SIZE 44
#define BUFFER_SAMPLES  256

// ================= STATE =================
const uint8_t* currentWav = nullptr;
uint32_t wavSize = 0;
uint32_t wavIndex = WAV_HEADER_SIZE;
bool playing = false;

// ================= SETUP =================
void setup() {
  // ---- Matrix ----
  pinMode(ROW0, OUTPUT);
  digitalWrite(ROW0, HIGH);

  pinMode(COL0, INPUT_PULLUP);
  pinMode(COL1, INPUT_PULLUP);
  pinMode(COL2, INPUT_PULLUP);

  // ---- I2S ----
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
    .bck_io_num   = I2S_BCLK,
    .ws_io_num    = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num  = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pins);
}

// ================= LOOP =================
void loop() {
  // ---- Scan row ----
  digitalWrite(ROW0, LOW);
  delayMicroseconds(3);

  bool k0 = (digitalRead(COL0) == LOW);
  bool k1 = (digitalRead(COL1) == LOW);
  bool k2 = (digitalRead(COL2) == LOW);

  digitalWrite(ROW0, HIGH);

  // ---- Key select (priority) ----
  if (k0) {
    if (currentWav != synas4) {
      currentWav = synas4;
      wavSize = sizeof(synas4);
      wavIndex = WAV_HEADER_SIZE;
    }
    playing = true;
  }
  else if (k1) {
    if (currentWav != synds4) {
      currentWav = synds4;
      wavSize = sizeof(synds4);
      wavIndex = WAV_HEADER_SIZE;
    }
    playing = true;
  }
  else if (k2) {
    if (currentWav != sync5) {
      currentWav = sync5;
      wavSize = sizeof(sync5);
      wavIndex = WAV_HEADER_SIZE;
    }
    playing = true;
  }
  else {
    playing = false;
    return;
  }

  // ---- Audio streaming (SEAMLESS LOOP) ----
  int16_t buffer[BUFFER_SAMPLES];
  int samples = 0;

  while (samples < BUFFER_SAMPLES) {
    if (wavIndex >= wavSize) {
      wavIndex = WAV_HEADER_SIZE;  // <<< seamless loop FIX
    }

    buffer[samples++] =
      (int16_t)(currentWav[wavIndex] |
               (currentWav[wavIndex + 1] << 8));
    wavIndex += 2;
  }

  size_t bytesWritten;
  i2s_write(I2S_NUM_0, buffer, samples * 2, &bytesWritten, portMAX_DELAY);
}