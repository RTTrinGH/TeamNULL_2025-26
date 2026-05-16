#include <Wire.h>

void setup() {
  Wire.begin();
  Serial.begin(115200);
  delay(1000);
}

void loop() {

  Serial.println("TX");

  Wire.beginTransmission(8);
  byte err = Wire.endTransmission();

  Serial.print("ERR=");
  Serial.println(err);

  delay(500);
}