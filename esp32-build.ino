#include <driver/i2s.h>
#include <math.h>

#define I2S_BCLK 1
#define I2S_LRC  2
#define I2S_DOUT 3

#define SAMPLE_RATE 44100
#define NOTE_FREQ 220   // Try 110, 220, 440

float phase1 = 0, phase2 = 0;
float env = 0;

void setup() {
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
  size_t bytes_written;

  // Envelope (slow attack)
  env += 0.0008;
  if (env > 1.0) env = 1.0;

  phase1 += 2.0 * PI * NOTE_FREQ / SAMPLE_RATE;
  phase2 += 2.0 * PI * NOTE_FREQ * 2 / SAMPLE_RATE;

  if (phase1 > 2 * PI) phase1 -= 2 * PI;
  if (phase2 > 2 * PI) phase2 -= 2 * PI;

  float tone =
      sin(phase1) * 0.7 +
      sin(phase2) * 0.3;

  static float lp = 0;
  lp += 0.02 * (tone - lp);

  int16_t sample = (int16_t)(lp * env * 14000);

  i2s_write(I2S_NUM_0, &sample, sizeof(sample), &bytes_written, portMAX_DELAY);
}