#include <driver/i2s.h>
#include <math.h>

#define I2S_BCLK 1
#define I2S_LRC  2
#define I2S_DOUT 3

#define SAMPLE_RATE 44100

// ===== SONG DATA =====
const float melody[] = {
  262, 262, 392, 392, 440, 440, 392,   // Twinkle
  349, 349, 330, 330, 294, 294, 262,
  392, 392, 349, 349, 330, 330, 294,
  392, 392, 349, 349, 330, 330, 294,
  262, 262, 392, 392, 440, 440, 392,
  349, 349, 330, 330, 294, 294, 262
};

const int noteDurations[] = {
  400,400,400,400,400,400,800,
  400,400,400,400,400,400,800,
  400,400,400,400,400,400,800,
  400,400,400,400,400,400,800,
  400,400,400,400,400,400,800,
  400,400,400,400,400,400,1000
};

int noteIndex = 0;
unsigned long noteStart = 0;

// ===== SYNTH =====
float phase = 0;
float env = 0;
float lp = 0;

inline float squareWave(float p) {
  return (sinf(p) > 0) ? 1.0f : -1.0f;
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

  noteStart = millis();
}

void loop() {
  size_t bw;

  // Move to next note
  if (millis() - noteStart > noteDurations[noteIndex]) {
    noteIndex++;
    if (noteIndex >= (sizeof(melody) / sizeof(float))) {
      noteIndex = 0; // loop song
    }
    noteStart = millis();
    env = 0; // retrigger envelope
    phase = 0;
  }

  float freq = melody[noteIndex];

  // Envelope (organ style)
  if (env < 1.0f) env += 0.002f;

  // Phase
  phase += 2 * PI * freq / SAMPLE_RATE;
  if (phase > 2 * PI) phase -= 2 * PI;

  // Square wave
  float tone = squareWave(phase);

  // Low-pass filter (smooth, warm)
  lp += 0.02f * (tone - lp);

  // Output (safe volume)
  int16_t sample = (int16_t)(lp * env * 6000);

  i2s_write(I2S_NUM_0, &sample, sizeof(sample), &bw, portMAX_DELAY);
}