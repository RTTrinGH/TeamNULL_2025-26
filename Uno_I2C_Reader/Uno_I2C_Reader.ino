#include <Wire.h>

static const uint8_t I2C_ADDRESS = 0x08;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  delay(1000);
  Serial.println("UNO I2C reader started");
}

void loop() {
  Wire.requestFrom(I2C_ADDRESS, (uint8_t)1);

  if (Wire.available()) {
    uint8_t value = Wire.read();
    Serial.println(value);
  }

  delay(50);
}