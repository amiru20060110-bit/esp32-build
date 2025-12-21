#include <driver/i2s.h>
#include <math.h>

#define I2S_BCLK 1
#define I2S_LRC  2
#define I2S_DOUT 3

#define SAMPLE_RATE 44100
#define NOTE_FREQ 220.0f

float p1 = 0, p2 = 0;
float env = 0;

float lp = 0;
float dc_prev = 0;
float dc_out = 0;

inline float softSquare(float phase) {
  return tanh(sinf(phase) * 2.5f);
}

void setup() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
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
  i2s_zero_dma_buffer(I2S_NUM_0);
}

void loop() {
  size_t bw;

  if (env < 1.0f) env += 0.0004f;

  p1 += 2 * PI * NOTE_FREQ / SAMPLE_RATE;
  p2 += 2 * PI * (NOTE_FREQ * 1.005f) / SAMPLE_RATE;

  if (p1 > 2 * PI) p1 -= 2 * PI;
  if (p2 > 2 * PI) p2 -= 2 * PI;

  float tone =
    softSquare(p1) * 0.7f +
    softSquare(p2) * 0.3f;

  lp += 0.02f * (tone - lp);

  dc_out = lp - dc_prev + 0.995f * dc_out;
  dc_prev = lp;

  //  50% volume (was 14000, now 7000)
  int16_t sample = (int16_t)(dc_out * env * 7000);

  i2s_write(I2S_NUM_0, &sample, sizeof(sample), &bw, portMAX_DELAY);
}