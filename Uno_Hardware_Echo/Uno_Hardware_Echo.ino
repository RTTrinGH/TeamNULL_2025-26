#include <Arduino.h>

const unsigned long BAUD = 115200;

void setup() {
  Serial.begin(BAUD);
  delay(100);
  Serial.println("UNO Hardware Echo started");
}

void loop() {
  while (Serial.available()) {
    int v = Serial.read();
    if (v < 0) continue;
    Serial.print("RX(0x");
    if ((uint8_t)v < 16) Serial.print('0');
    Serial.print((uint8_t)v, HEX);
    Serial.print(") '");
    if (v >= 32 && v <= 126) Serial.print((char)v);
    else Serial.print('.');
    Serial.println("'");
  }
}
