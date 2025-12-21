#include <driver/i2s.h>

// ================= AUDIO =================
#define I2S_BCLK 1
#define I2S_LRC  2
#define I2S_DOUT 3

#define SAMPLE_RATE 44100
#define VOLUME 14000   // Loud but clean

// ================= MATRIX =================
const int rowPins[5] = {15, 16, 17, 18, 38};
const int colPins[6] = {40, 41, 42, 44, 39, 43};

// ================= NOTES (C3 → F5) =================
const float noteFreq[30] = {
  130.81, 138.59, 146.83, 155.56, 164.81, 174.61,
  185.00, 196.00, 207.65, 220.00, 233.08, 246.94,

  261.63, 277.18, 293.66, 311.13, 329.63, 349.23,
  369.99, 392.00, 415.30, 440.00, 466.16, 493.88,

  523.25, 554.37, 587.33, 622.25, 659.25, 698.46
};

// ================= SYNTH =================
volatile float phase = 0.0f;
volatile float freq  = 0.0f;

// ================= SETUP =================
void setup() {
  // Rows as outputs
  for (int i = 0; i < 5; i++) {
    pinMode(rowPins[i], OUTPUT);
    digitalWrite(rowPins[i], HIGH);
  }

  // Columns as inputs with pulldown
  for (int i = 0; i < 6; i++) {
    pinMode(colPins[i], INPUT_PULLDOWN);
  }

  // I2S configuration
  i2s_config_t i2s_config = {
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

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
}

// ================= LOOP =================
void loop() {
  scanKeyboard();
  generateAudio();
}

// ================= KEYBOARD SCAN =================
void scanKeyboard() {
  freq = 0;

  for (int r = 0; r < 5; r++) {
    digitalWrite(rowPins[r], LOW);
    delayMicroseconds(3);

    for (int c = 0; c < 6; c++) {
      if (digitalRead(colPins[c])) {
        freq = noteFreq[r * 6 + c];
      }
    }

    digitalWrite(rowPins[r], HIGH);
  }
}

// ================= AUDIO ENGINE =================
void generateAudio() {
  int16_t sample;
  size_t bytesWritten;

  if (freq > 0) {
    phase += freq / SAMPLE_RATE;
    if (phase >= 1.0f) phase -= 1.0f;

    // Pure square wave (cleanest sound)
    sample = (phase < 0.5f) ? VOLUME : -VOLUME;
  } else {
    sample = 0;
  }

  i2s_write(I2S_NUM_0, &sample, sizeof(sample), &bytesWritten, portMAX_DELAY);
}