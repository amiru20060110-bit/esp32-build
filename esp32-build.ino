#include <Arduino.h>
#include <driver/i2s.h>

#include "ce4.h"   // Piano C4
#include "cs4.h"   // Piano C#4
#include "d4.h"    // Piano D4

// ================= I2S =================
#define I2S_BCLK 1
#define I2S_LRC  2
#define I2S_DOUT 3

// ================= MATRIX ==============
#define COL0 40

#define ROW_CE4 15
#define ROW_CS4 16
#define ROW_D4  17

// ================= AUDIO ===============
#define SAMPLE_RATE     32000
#define WAV_HEADER_SIZE 44
#define BUFFER_SAMPLES  256

const uint8_t* current_wav = nullptr;
uint32_t wav_size = 0;
uint32_t wav_pos = WAV_HEADER_SIZE;

bool key_active = false;
bool sample_done = false;

// ================= SETUP ================
void setup() {
  pinMode(COL0, INPUT_PULLUP);

  pinMode(ROW_CE4, OUTPUT);
  pinMode(ROW_CS4, OUTPUT);
  pinMode(ROW_D4,  OUTPUT);

  digitalWrite(ROW_CE4, HIGH);
  digitalWrite(ROW_CS4, HIGH);
  digitalWrite(ROW_D4,  HIGH);

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
    .ws_io_num  = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num  = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pins);
}

// ================= LOOP =================
void loop() {
  const uint8_t* selected_wav = nullptr;
  uint32_t selected_size = 0;
  bool key_pressed = false;

  // --- C4 ---
  digitalWrite(ROW_CE4, LOW);
  delayMicroseconds(3);
  if (digitalRead(COL0) == LOW) {
    selected_wav = ce4;
    selected_size = sizeof(ce4);
    key_pressed = true;
  }
  digitalWrite(ROW_CE4, HIGH);

  // --- C#4 ---
  digitalWrite(ROW_CS4, LOW);
  delayMicroseconds(3);
  if (digitalRead(COL0) == LOW) {
    selected_wav = cs4;
    selected_size = sizeof(cs4);
    key_pressed = true;
  }
  digitalWrite(ROW_CS4, HIGH);

  // --- D4 ---
  digitalWrite(ROW_D4, LOW);
  delayMicroseconds(3);
  if (digitalRead(COL0) == LOW) {
    selected_wav = d4;
    selected_size = sizeof(d4);
    key_pressed = true;
  }
  digitalWrite(ROW_D4, HIGH);

  // ---- Key released ----
  if (!key_pressed) {
    key_active = false;
    sample_done = false;
    return;
  }

  // ---- New key press ----
  if (!key_active) {
    current_wav = selected_wav;
    wav_size = selected_size;
    wav_pos = WAV_HEADER_SIZE;
    key_active = true;
    sample_done = false;
  }

  // ---- Sample finished ----
  if (sample_done) return;

  int16_t buffer[BUFFER_SAMPLES];
  int samples = 0;

  while (samples < BUFFER_SAMPLES) {
    if (wav_pos >= wav_size - 2) {
      sample_done = true;
      break;
    }

    buffer[samples++] = (int16_t)(
      current_wav[wav_pos] |
      (current_wav[wav_pos + 1] << 8)
    );

    wav_pos += 2;
  }

  if (samples > 0) {
    size_t bw;
    i2s_write(I2S_NUM_0, buffer, samples * 2, &bw, portMAX_DELAY);
  }
}