#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <driver/i2s.h>

// ================= I2S PINS =================
#define I2S_BCLK 1
#define I2S_LRC  2
#define I2S_DOUT 3

// ================= SD PINS ==================
#define SD_CS   4
#define SD_MOSI 6
#define SD_MISO 5
#define SD_CLK  7

// ================= MATRIX PINS ==============
#define COL0 40
#define ROW0 15

// ================= AUDIO ===================
#define SAMPLE_RATE 32000
#define BUFFER_SAMPLES 256

File wavFile;
int16_t buffer[BUFFER_SAMPLES];

void setup() {
  Serial.begin(115200);

  // Column and row
  pinMode(COL0, OUTPUT);
  digitalWrite(COL0, HIGH);
  pinMode(ROW0, INPUT_PULLUP);

  // Initialize SD
  SPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS)) {
    Serial.println("SD Card init failed!");
    while (1);
  }
  Serial.println("SD ready.");

  // Initialize I2S
  i2s_config_t cfg = {
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

  i2s_pin_config_t pins = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num  = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num  = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM_0, &cfg, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pins);
}

void loop() {
  // Scan key
  digitalWrite(COL0, LOW);
  delayMicroseconds(3);
  if (digitalRead(ROW0) == LOW) {
    // Play WAV once
    if (SD.exists("ce4.wav")) {
      wavFile = SD.open("ce4.wav");
      if (wavFile) {
        wavFile.seek(44); // skip header
        while (wavFile.available()) {
          int samplesRead = 0;
          while (samplesRead < BUFFER_SAMPLES && wavFile.available()) {
            int16_t s = wavFile.read() | (wavFile.read() << 8);
            buffer[samplesRead++] = s;
          }
          size_t bw;
          i2s_write(I2S_NUM_0, buffer, samplesRead * 2, &bw, portMAX_DELAY);
        }
        wavFile.close();
      }
    }
  }
  digitalWrite(COL0, HIGH);
}