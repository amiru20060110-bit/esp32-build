#include <driver/i2s.h>

// ===== I2S =====
#define I2S_BCLK 1
#define I2S_LRC  2
#define I2S_DOUT 3

// ===== MATRIX =====
#define ROW0 15
#define COL0 40   // A3
#define COL1 41   // A5

#define SAMPLE_RATE 44100
#define VOLUME 12000

#define FREQ_IDLE 440.0f
#define FREQ_A3   220.0f
#define FREQ_A5   880.0f

float phase = 0.0f;

void setup() {
  // Row
  pinMode(ROW0, OUTPUT);
  digitalWrite(ROW0, HIGH);

  // Columns
  pinMode(COL0, INPUT_PULLDOWN);
  pinMode(COL1, INPUT_PULLDOWN);

  // I2S
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

void loop() {
  size_t bw;

  float freq = FREQ_IDLE;

  // Scan row
  digitalWrite(ROW0, LOW);
  delayMicroseconds(3);

  if (digitalRead(COL0)) freq = FREQ_A3;
  if (digitalRead(COL1)) freq = FREQ_A5;

  digitalWrite(ROW0, HIGH);

  // Audio
  phase += freq / SAMPLE_RATE;
  if (phase >= 1.0f) phase -= 1.0f;

  int16_t sample = (phase < 0.5f) ? VOLUME : -VOLUME;
  i2s_write(I2S_NUM_0, &sample, sizeof(sample), &bw, portMAX_DELAY);
}