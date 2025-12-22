#include <Arduino.h>
#include <driver/i2s.h>

#include "synas4.h"  // AS4
#include "synds4.h"  // DS4
#include "sync5.h"   // C5

// ===== I2S PINS =====
#define I2S_BCLK 1
#define I2S_LRC  2
#define I2S_DOUT 3

// ===== MATRIX PINS =====
#define COL0 40   // original key column
#define COL1 41   // new pitch-shift key

#define ROW_AS4 15
#define ROW_DS4 16
#define ROW_C5  17

// ===== AUDIO =====
#define SAMPLE_RATE 44100
#define WAV_HEADER_SIZE 44
#define BUFFER_SAMPLES 256

// Pitch shifting factor for new key (example: A4 = half step below AS4)
#define PITCH_SHIFT 0.94387f  // e.g., A4 from AS4

const uint8_t* current_wav = nullptr;
uint32_t wav_size = 0;
float wav_pos = WAV_HEADER_SIZE;
bool playing = false;

// ===== SETUP =====
void setup() {
  // Columns
  pinMode(COL0, INPUT_PULLUP);
  pinMode(COL1, INPUT_PULLUP);

  // Rows
  pinMode(ROW_AS4, OUTPUT);
  pinMode(ROW_DS4, OUTPUT);
  pinMode(ROW_C5, OUTPUT);

  digitalWrite(ROW_AS4, HIGH);
  digitalWrite(ROW_DS4, HIGH);
  digitalWrite(ROW_C5, HIGH);

  // I2S
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

// ===== LOOP =====
void loop() {
  // Scan keys (row->column)
  current_wav = nullptr;

  // AS4
  digitalWrite(ROW_AS4, LOW);
  delayMicroseconds(3);
  if (digitalRead(COL0) == LOW) {
    current_wav = synas4;
    wav_size = sizeof(synas4);
  }
  digitalWrite(ROW_AS4, HIGH);

  // DS4
  digitalWrite(ROW_DS4, LOW);
  delayMicroseconds(3);
  if (digitalRead(COL0) == LOW) {
    current_wav = synds4;
    wav_size = sizeof(synds4);
  }
  digitalWrite(ROW_DS4, HIGH);

  // C5
  digitalWrite(ROW_C5, LOW);
  delayMicroseconds(3);
  if (digitalRead(COL0) == LOW) {
    current_wav = sync5;
    wav_size = sizeof(sync5);
  }
  digitalWrite(ROW_C5, HIGH);

  // New pitch-shift key
  digitalWrite(ROW_AS4, LOW); // we use AS4 WAV for pitch shift
  delayMicroseconds(3);
  if (digitalRead(COL1) == LOW) {
    current_wav = synas4; // use AS4 sample
    wav_size = sizeof(synas4);
  }
  digitalWrite(ROW_AS4, HIGH);

  // If no key pressed, do nothing
  if (!current_wav) {
    playing = false;
    return;
  }

  // Start playback once
  if (!playing) {
    wav_pos = WAV_HEADER_SIZE;
    playing = true;
  }

  int16_t buffer[BUFFER_SAMPLES];
  int samples = 0;

  while (samples < BUFFER_SAMPLES) {
    // Loop WAV
    if ((int)wav_pos >= wav_size - 2) {
      wav_pos = WAV_HEADER_SIZE;
    }

    int idx = (int)wav_pos;
    int16_t s = (int16_t)(current_wav[idx] | (current_wav[idx + 1] << 8));
    buffer[samples++] = s;

    // Apply pitch shift if pitch-shift column
    if (digitalRead(COL1) == LOW) {
      wav_pos += PITCH_SHIFT * 2.0f;
    } else {
      wav_pos += 2.0f;
    }
  }

  size_t bw;
  i2s_write(I2S_NUM_0, buffer, samples * 2, &bw, portMAX_DELAY);
}