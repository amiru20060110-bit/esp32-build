#include <Arduino.h>
#include "driver/i2s.h"
#include <math.h>

// ===== I2S PINS =====
#define I2S_BCLK  35
#define I2S_LRC   34
#define I2S_DOUT  33

// ===== AUDIO SETTINGS =====
#define SAMPLE_RATE 44100
#define TONE_FREQ   440      // A4 note
#define AMPLITUDE   20000    // Volume (max ~32767)

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32-S3 I2S Tone Test");

  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 256,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);

  Serial.println("I2S started");
}

void loop() {
  static float phase = 0;
  int16_t samples[256];
  size_t bytes_written;

  for (int i = 0; i < 256; i++) {
    samples[i] = (int16_t)(AMPLITUDE * sin(phase));
    phase += 2.0 * PI * TONE_FREQ / SAMPLE_RATE;
    if (phase > 2.0 * PI) phase -= 2.0 * PI;
  }

  i2s_write(I2S_NUM_0, samples, sizeof(samples), &bytes_written, portMAX_DELAY);
}