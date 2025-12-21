#include <driver/i2s.h>

// ===== I2S PINS =====
#define I2S_BCLK 1
#define I2S_LRC  2
#define I2S_DOUT 3

// ===== BUTTON =====
#define KEY_PIN 15   // Button to GND

#define SAMPLE_RATE 44100
#define VOLUME 12000

// Frequencies
#define FREQ_IDLE 440.0f   // A4 (always playing)
#define FREQ_KEY  220.0f   // A3 (when pressed)

float phase = 0.0f;

void setup() {
  pinMode(KEY_PIN, INPUT_PULLUP);

  i2s_config_t i2s_config = {
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

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
}

void loop() {
  size_t bw;

  // Select frequency
  float freq = (digitalRead(KEY_PIN) == LOW) ? FREQ_KEY : FREQ_IDLE;

  // Phase accumulator
  phase += freq / SAMPLE_RATE;
  if (phase >= 1.0f) phase -= 1.0f;

  // Square wave output
  int16_t sample = (phase < 0.5f) ? VOLUME : -VOLUME;

  i2s_write(I2S_NUM_0, &sample, sizeof(sample), &bw, portMAX_DELAY);
}