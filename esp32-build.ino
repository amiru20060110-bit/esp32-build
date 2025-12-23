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
#define COL1 41

#define ROW0 15
#define ROW1 16
#define ROW2 17

// ================= AUDIO ===================
#define SAMPLE_RATE 32000
#define BUFFER_SAMPLES 256

File wavFile;
int16_t buffer[BUFFER_SAMPLES];

// ================= SETUP ===================
void setup() {
  Serial.begin(115200);

  // Matrix columns
  pinMode(COL0, OUTPUT);
  pinMode(COL1, OUTPUT);
  digitalWrite(COL0, HIGH);
  digitalWrite(COL1, HIGH);

  // Matrix rows
  pinMode(ROW0, INPUT_PULLUP);
  pinMode(ROW1, INPUT_PULLUP);
  pinMode(ROW2, INPUT_PULLUP);

  // Initialize SD card
  SPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);
  if (!SD.begin(SD_CS)) {
    Serial.println("SD Card initialization failed!");
    while (1);
  }
  Serial.println("SD Card ready.");

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

// ================= PLAYBACK FUNCTION =========
void playWav(const char* filename) {
  if (!SD.exists(filename)) return;

  wavFile = SD.open(filename);
  if (!wavFile) return;

  // Skip WAV header
  wavFile.seek(44);

  while (wavFile.available()) {
    int samplesRead = 0;
    while (samplesRead < BUFFER_SAMPLES && wavFile.available()) {
      int16_t s = wavFile.read() | (wavFile.read() << 8);
      buffer[samplesRead++] = s;
    }
    size_t bytesWritten;
    i2s_write(I2S_NUM_0, buffer, samplesRead * 2, &bytesWritten, portMAX_DELAY);
  }
  wavFile.close();
}

// ================= LOOP ======================
void loop() {
  // Scan column 0
  digitalWrite(COL0, LOW);
  delayMicroseconds(3);
  if (digitalRead(ROW0) == LOW) playWav("ce4.wav");
  else if (digitalRead(ROW1) == LOW) playWav("cs4.wav");
  else if (digitalRead(ROW2) == LOW) playWav("d4.wav");
  digitalWrite(COL0, HIGH);

  // Scan column 1
  digitalWrite(COL1, LOW);
  delayMicroseconds(3);
  if (digitalRead(ROW0) == LOW) playWav("ce4.wav");
  else if (digitalRead(ROW1) == LOW) playWav("cs4.wav");
  else if (digitalRead(ROW2) == LOW) playWav("d4.wav");
  digitalWrite(COL1, HIGH);
}