#include <driver/i2s.h>

#define I2S_BCLK  1
#define I2S_LRC   2
#define I2S_DOUT  3

#define SAMPLE_RATE 44100
#define AMPLITUDE   26000   //  louder (max safe ≈ 28000)

struct Note {
  int freq;
  int duration;
};

Note melody[] = {
  {262, 400}, {294, 400}, {330, 400}, {349, 400},
  {392, 400}, {440, 400}, {494, 400}, {523, 800},
  {0,   400}
};

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

void playSquare(int freq, int durationMs) {
  if (freq == 0) {
    delay(durationMs);
    return;
  }

  int samples = SAMPLE_RATE * durationMs / 1000;
  int period = SAMPLE_RATE / freq;

  int16_t buffer[256];
  size_t bytes_written;

  for (int i = 0; i < samples; i += 256) {
    for (int j = 0; j < 256; j++) {
      int phase = (i + j) % period;
      buffer[j] = (phase < period / 2) ? AMPLITUDE : -AMPLITUDE;
    }
    i2s_write(I2S_NUM_0, buffer, sizeof(buffer), &bytes_written, portMAX_DELAY);
  }
}

void loop() {
  for (int i = 0; i < sizeof(melody) / sizeof(Note); i++) {
    playSquare(melody[i].freq, melody[i].duration);
    delay(40);
  }
  delay(2000);
}