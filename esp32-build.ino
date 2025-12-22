#include <driver/i2s.h>
#include <math.h>

// ================= I2S =================
#define I2S_BCLK  1
#define I2S_LRC   2
#define I2S_DOUT  3

#define SAMPLE_RATE 44100
#define VOLUME 12000

// ================= MATRIX =================
const int rowPins[4] = {15, 16, 17, 18};
const int colPins[4] = {21, 22, 23, 24};

// ================= WAV DATA =================
// (replace with your actual PROGMEM arrays)
extern const int16_t c5_wav[];
extern const uint32_t c5_len;

extern const int16_t ds4_wav[];
extern const uint32_t ds4_len;

extern const int16_t as4_wav[];
extern const uint32_t as4_len;

// ================= KEY TABLE =================
struct Key {
  const int16_t* data;
  uint32_t len;
  float pitch;
};

Key keys[13] = {
  {ds4_wav, ds4_len, pow(2, -3/12.0)}, // C4
  {ds4_wav, ds4_len, pow(2, -2/12.0)}, // C#4
  {ds4_wav, ds4_len, pow(2, -1/12.0)}, // D4
  {ds4_wav, ds4_len, 1.0},             // D#4
  {ds4_wav, ds4_len, pow(2,  1/12.0)}, // E4

  {as4_wav, as4_len, pow(2, -5/12.0)}, // F4
  {as4_wav, as4_len, pow(2, -4/12.0)}, // F#4
  {as4_wav, as4_len, pow(2, -3/12.0)}, // G4
  {as4_wav, as4_len, pow(2, -2/12.0)}, // G#4
  {as4_wav, as4_len, pow(2, -1/12.0)}, // A4
  {as4_wav, as4_len, 1.0},             // A#4

  {c5_wav,  c5_len,  pow(2, -1/12.0)}, // B4
  {c5_wav,  c5_len,  1.0},             // C5
};

int activeKey = -1;
float phase = 0;

// ================= SETUP =================
void setup() {
  // Matrix
  for (int r = 0; r < 4; r++) {
    pinMode(rowPins[r], OUTPUT);
    digitalWrite(rowPins[r], HIGH);
  }
  for (int c = 0; c < 4; c++) {
    pinMode(colPins[c], INPUT_PULLUP);
  }

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
    .ws_io_num  = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num  = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pins);
}

// ================= LOOP =================
void loop() {
  scanKeys();
  playAudio();
}

// ================= MATRIX SCAN =================
void scanKeys() {
  activeKey = -1;
  int keyIndex = 0;

  for (int r = 0; r < 4; r++) {
    digitalWrite(rowPins[r], LOW);
    delayMicroseconds(3);

    for (int c = 0; c < 4; c++) {
      if (digitalRead(colPins[c]) == LOW && keyIndex < 13) {
        activeKey = keyIndex;
        digitalWrite(rowPins[r], HIGH);
        return;
      }
      keyIndex++;
    }
    digitalWrite(rowPins[r], HIGH);
  }
}

// ================= AUDIO =================
void playAudio() {
  int16_t sample = 0;
  size_t bw;

  if (activeKey >= 0) {
    Key &k = keys[activeKey];
    phase += k.pitch;
    if (phase >= k.len) phase -= k.len;
    sample = k.data[(int)phase];
  }

  i2s_write(I2S_NUM_0, &sample, sizeof(sample), &bw, portMAX_DELAY);
}