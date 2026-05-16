#include <SoftwareSerial.h>

const unsigned long BAUD = 115200;
// Use pin 9 as RX (Micro TX -> Uno pin 9). TX not used.
SoftwareSerial uart(9, -1);

void setup() {
  Serial.begin(BAUD);
  uart.begin(BAUD);
  delay(100);
  Serial.println("UNO UART reader started (SoftwareSerial pin 9)");
}

void loop() {
  while (uart.available()) {
    int v = uart.read();
    Serial.print("RX char: ");
    Serial.print((char)v);
    Serial.print("  (0x");
    if (v < 16) Serial.print('0');
    Serial.print(v, HEX);
    Serial.println(")");
  }
}
