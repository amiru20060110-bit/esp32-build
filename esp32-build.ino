#include <Arduino.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "driver/i2s.h"

// SD Card Pins
#define SD_MOSI 6
#define SD_MISO 5
#define SD_CLK  7
#define SD_CS   4

// I2S Pins
#define I2S_BCLK 1
#define I2S_LRC  2
#define I2S_DOUT 3

// Audio Config
#define SAMPLE_RATE     32000
#define I2S_NUM         I2S_NUM_0
#define DMA_BUF_COUNT   8
#define DMA_BUF_LEN     1024

void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT, // Mono
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = DMA_BUF_COUNT,
    .dma_buf_len = DMA_BUF_LEN,
    .use_apll = false
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM, &pin_config);
}

void playWAV(const char* path) {
  File file = SD.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  // Skip WAV header (first 44 bytes) to reach raw PCM data
  file.seek(44);

  static uint8_t buf[1024];
  size_t bytesRead = 0;
  size_t bytesWritten = 0;

  Serial.println("Playing...");

  while (file.available()) {
    bytesRead = file.read(buf, sizeof(buf));
    // Write to I2S DMA buffer
    i2s_write(I2S_NUM, buf, bytesRead, &bytesWritten, portMAX_DELAY);
  }

  file.close();
  Serial.println("Finished playback.");
}

void setup() {
  Serial.begin(115200);

  // Initialize SPI for SD Card
  SPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);
  
  if (!SD.begin(SD_CS)) {
    Serial.println("SD Card Mount Failed");
    return;
  }

  setupI2S();
  playWAV("/ce4.wav");
}

void loop() {
  // Idle
}
