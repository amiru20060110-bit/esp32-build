#define ARDUINO_USB_CDC_ON_BOOT 1

void setup() {
  Serial.begin(115200);
}

void loop() {
  if (Serial.available()) {
    Serial.write(Serial.read()); // echo
  }
}