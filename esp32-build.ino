void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("ESP32-S3 is running!");
}

void loop() {
  Serial.println("Loop alive...");
  delay(1000);
}
