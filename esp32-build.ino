#define SPEAKER_PIN 4
#define CHANNEL 0
#define RESOLUTION 8   // 8-bit PWM

void setup() {
  ledcSetup(CHANNEL, 1000, RESOLUTION); // 1000 Hz tone
  ledcAttachPin(SPEAKER_PIN, CHANNEL);
}

void loop() {
  // Play note
  ledcWrite(CHANNEL, 128);  // 50% duty → sound ON
  delay(500);

  // Silence
  ledcWrite(CHANNEL, 0);    // sound OFF
  delay(500);
}