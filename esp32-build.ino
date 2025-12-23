#include <Arduino.h>
#include <SD.h>
#include <driver/i2s.h>

// ===== SD CARD PINS =====
#define SD_MOSI 6
#define SD_MISO 5
#define SD_CLK  7
#define SD_CS   4

// ===== I2S PINS =====
#define I2S_BCLK 1
#define I2S_LRC  2
#define I2S_DOUT 3

// ===== AUDIO =====
#define SAMPLE_RATE 32000
#define BUFFER_SAMPLES 256

File wavFile;
int16_t buffer[BUFFER_SAMPLES];

void setup() {
  Serial.begin(115200);

  // ----- SD -----
  if (!SD.begin(SD_CS)) {
    Serial.println("SD Card init failed!");
    while (1);
  }
  Serial.println("SD Card ready");

  wavFile = SD.open("/ce4.wav");
  if (!wavFile) {
    Serial.println("Failed to open ce4.wav");
    while (1);
  }

  // Skip WAV header (44 bytes)
  wavFile.seek(44);

  // ----- I2S -----
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

  i2s_pin_config_t i2s_pins = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num  = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num  = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &i2s_pins);
}

void loop() {
  int samplesRead = 0;

  while (samplesRead < BUFFER_SAMPLES) {
    if (wavFile.available()) {
      int16_t sample = wavFile.read() | (wavFile.read() << 8);
      buffer[samplesRead++] = sample;
    } else {
      // Rewind to start after reaching end
      wavFile.seek(44);
    }
  }

  size_t bytesWritten;
  i2s_write(I2S_NUM_0, buffer, samplesRead * 2, &bytesWritten, portMAX_DELAY);
}