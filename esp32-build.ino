#define SPEAKER_PIN 17

void setup() {
  // Attach PWM to pin, 1000 Hz, 8-bit resolution
  ledcAttach(SPEAKER_PIN, 1000, 8);
}

void loop() {
  // Play tone
  ledcWrite(SPEAKER_PIN, 20);  // LOW volume (safe)
  delay(500);

  // Silence
  ledcWrite(SPEAKER_PIN, 0);
  delay(500);
}