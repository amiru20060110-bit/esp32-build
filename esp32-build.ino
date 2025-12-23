#include <Arduino.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include "driver/i2s.h"

// SD Card Pins (SPI)
#define SD_MOSI 6
#define SD_MISO 5
#define SD_CLK  7
#define SD_CS   4

// I2S Pins
#define I2S_BCLK 1
#define I2S_LRC  2
#define I2S_DOUT 3

// Matrix Pins (Updated Logic: Column to Row)
#define COL_PIN 40  // Now the Output Source
#define ROW_PIN 15  // Now the Input Sink

// Audio Config
#define SAMPLE_RATE 32000
#define I2S_NUM     I2S_NUM_0

File audioFile;
bool isPlaying = false;

void setupI2S() {
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 512, 
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

void setup() {
  Serial.begin(115200);

  // Matrix Setup for Column -> Row
  pinMode(COL_PIN, OUTPUT);
  pinMode(ROW_PIN, INPUT_PULLDOWN);
  digitalWrite(COL_PIN, LOW); // Stay low when not checking

  // SD Setup
  SPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS)) {
    Serial.println("SD Card Mount Failed");
    while(1);
  }
  
  setupI2S();
  Serial.println("Ready. Diode Orientation: Col -> Row");
}

void loop() {
  // --- Matrix Scanning (Col -> Row) ---
  digitalWrite(COL_PIN, HIGH);        // Pulse the Column
  bool keyPressed = (digitalRead(ROW_PIN) == HIGH); // Check the Row
  digitalWrite(COL_PIN, LOW);         // Reset Column

  if (keyPressed) {
    if (!isPlaying) {
      audioFile = SD.open("/ce4.wav");
      if (audioFile) {
        audioFile.seek(44); // Skip header
        isPlaying = true;
      }
    }

    if (isPlaying && audioFile.available()) {
      static uint8_t buf[512];
      size_t bytesRead = audioFile.read(buf, sizeof(buf));
      size_t bytesWritten;
      i2s_write(I2S_NUM, buf, bytesRead, &bytesWritten, portMAX_DELAY);
    } else if (isPlaying && !audioFile.available()) {
      audioFile.seek(44); // Loop if still holding key
    }
  } 
  else {
    // Immediate stop on release
    if (isPlaying) {
      isPlaying = false;
      audioFile.close();
      i2s_zero_dma_buffer(I2S_NUM); 
    }
  }
}
