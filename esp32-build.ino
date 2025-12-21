#include <driver/i2s.h>
#include <math.h>

#define I2S_BCLK 1
#define I2S_LRC  2
#define I2S_DOUT 3

#define SAMPLE_RATE 44100
#define NOTE_FREQ 220
#define BUF_LEN 256

float p1 = 0, p2 = 0, p3 = 0;
float env = 0;

int16_t buffer[BUF_LEN];

void setup() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = BUF_LEN,
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
  i2s_zero_dma_buffer(I2S_NUM_0);
}

void loop() {
  // VERY slow attack (organ-like)
  env += 0.0002;
  if (env > 1.0) env = 1.0;

  for (int i = 0; i < BUF_LEN; i++) {
    p1 += 2 * PI * NOTE_FREQ / SAMPLE_RATE;
    p2 += 2 * PI * (NOTE_FREQ * 1.003) / SAMPLE_RATE;
    p3 += 2 * PI * (NOTE_FREQ * 0.997) / SAMPLE_RATE;

    if (p1 > 2 * PI) p1 -= 2 * PI;
    if (p2 > 2 * PI) p2 -= 2 * PI;
    if (p3 > 2 * PI) p3 -= 2 * PI;

    float tone =
        sin(p1) * 0.6 +
        sin(p2) * 0.2 +
        sin(p3) * 0.2;

    static float lp = 0;
    lp += 0.01 * (tone - lp);

    buffer[i] = (int16_t)(lp * env * 15000);
  }

  size_t bytes_written;
  i2s_write(I2S_NUM_0, buffer, sizeof(buffer), &bytes_written, portMAX_DELAY);
}