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

// Matrix Pins
#define ROW_PIN 15
#define COL_PIN 40

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
    .dma_buf_len = 512, // Smaller buffer for better responsiveness
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

  // Matrix Setup
  pinMode(ROW_PIN, OUTPUT);
  pinMode(COL_PIN, INPUT_PULLDOWN);
  digitalWrite(ROW_PIN, LOW); // Keep low until scanning

  // SD Setup
  SPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS)) {
    Serial.println("SD Card Mount Failed");
    while(1);
  }
  
  setupI2S();
  Serial.println("System Ready. Press Key to play.");
}

void loop() {
  // --- Matrix Scanning ---
  digitalWrite(ROW_PIN, HIGH);       // Set Row High
  bool keyPressed = (digitalRead(COL_PIN) == HIGH); // Read Column
  digitalWrite(ROW_PIN, LOW);        // Set Row back to Low

  if (keyPressed) {
    if (!isPlaying) {
      // Start playing
      audioFile = SD.open("/ce4.wav");
      if (audioFile) {
        audioFile.seek(44); // Skip WAV header
        isPlaying = true;
        Serial.println("Key Pressed: Playing...");
      }
    }

    // If file is open, stream a chunk of data to I2S
    if (isPlaying && audioFile.available()) {
      static uint8_t buf[512];
      size_t bytesRead = audioFile.read(buf, sizeof(buf));
      size_t bytesWritten;
      i2s_write(I2S_NUM, buf, bytesRead, &bytesWritten, portMAX_DELAY);
    } else if (isPlaying && !audioFile.available()) {
      // Loop the sound if it reaches the end while key is still held
      audioFile.seek(44);
    }
  } 
  else {
    // Key Released
    if (isPlaying) {
      isPlaying = false;
      audioFile.close();
      i2s_zero_dma_buffer(I2S_NUM); // Immediately silence the output
      Serial.println("Key Released: Stopped.");
    }
  }
}
