#include <Wire.h>

static const uint8_t I2C_ADDRESS = 0x08;
volatile uint8_t counter = 0;

void onRequest() {
  Wire.write(counter);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_ADDRESS);
  Wire.onRequest(onRequest);
  Serial.println("Micro I2C ping slave ready at 0x08");
}

void loop() {
  static uint32_t lastTick = 0;
  uint32_t now = millis();
  if ((uint32_t)(now - lastTick) >= 500) {
    lastTick = now;

    counter++;
    Serial.print("counter=");
    Serial.println(counter);
  }
}
