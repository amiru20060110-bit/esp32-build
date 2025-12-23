#include <Arduino.h>
#include <driver/i2s.h>
#include <SD.h>
#include <SPI.h>

// ===== I2S PINS =====
#define I2S_BCLK 1
#define I2S_LRC  2
#define I2S_DOUT 3

// ===== MATRIX PINS =====
#define ROW0 15  // CE4
#define ROW1 16  // CS4
#define ROW2 17  // D4
#define COL0 40  // single column, active LOW

// ===== SD CARD PINS =====
#define SD_MOSI 6
#define SD_MISO 5
#define SD_CLK  7
#define SD_CS   4

// ===== AUDIO =====
#define SAMPLE_RATE 32000  // changed from 44100 to 32 kHz
#define BUFFER_SAMPLES 256

File wavFile;
int16_t buffer[BUFFER_SAMPLES];

// ===== SETUP =====
void setup() {
  // Matrix setup
  pinMode(ROW0, OUTPUT); digitalWrite(ROW0, HIGH);
  pinMode(ROW1, OUTPUT); digitalWrite(ROW1, HIGH);
  pinMode(ROW2, OUTPUT); digitalWrite(ROW2, HIGH);
  pinMode(COL0, INPUT_PULLUP);

  // SD card setup
  SPIClass spiSD(SD_MOSI, SD_MISO, SD_CLK, SD_CS);
  if (!SD.begin(SD_CS, spiSD)) {
    Serial.println("SD Card init failed!");
    while (1);
  }

  // I2S setup
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

// ===== LOOP =====
void loop() {
  File selectedFile;

  // Scan matrix
  int16_t rowPins[3] = {ROW0, ROW1, ROW2};
  const char* wavFiles[3] = {"/ce4.wav", "/cs4.wav", "/d4.wav"};
  bool keyPressed = false;

  for (int r = 0; r < 3; r++) {
    digitalWrite(rowPins[r], LOW);
    delayMicroseconds(3);
    if (digitalRead(COL0) == LOW) {
      selectedFile = SD.open(wavFiles[r]);
      keyPressed = true;
      digitalWrite(rowPins[r], HIGH);
      break;
    }
    digitalWrite(rowPins[r], HIGH);
  }

  if (!keyPressed) return;

  if (!selectedFile) return;

  // Skip WAV header (assume 44 bytes)
  selectedFile.seek(44);

  // Play audio until file ends
  while (selectedFile.available()) {
    int samplesRead = 0;
    while (samplesRead < BUFFER_SAMPLES && selectedFile.available()) {
      uint8_t lo = selectedFile.read();
      uint8_t hi = selectedFile.read();
      buffer[samplesRead++] = (int16_t)(lo | (hi << 8));
    }
    size_t bytesWritten;
    i2s_write(I2S_NUM_0, buffer, samplesRead * 2, &bytesWritten, portMAX_DELAY);
  }

  selectedFile.close();
}