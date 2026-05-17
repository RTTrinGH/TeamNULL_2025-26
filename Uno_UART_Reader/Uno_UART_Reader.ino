#include <SoftwareSerial.h>

const unsigned long BAUD = 115200;
// Use pin 9 as RX (Micro TX -> Uno pin 9). TX not used.
SoftwareSerial uart(0, 1);

int decodeCounterLabel(char label) {
  if (label >= 'A' && label <= 'L') {
    return label - 'A';
  }
  return -1;
}

void setup() {
  Serial.begin(BAUD);
  uart.begin(BAUD);
  delay(100);
  Serial.println("UNO UART reader started (SoftwareSerial pin 9)");
}

void loop() {
  while (uart.available()) {
    int v = uart.read();
    if (v == '\r' || v == '\n') {
      continue;
    }

    char label = (char)v;
    int counter = decodeCounterLabel(label);

    Serial.print("RX label: ");
    Serial.print(label);
    Serial.print("  counter: ");
    if (counter >= 0) {
      Serial.println(counter);
    } else {
      Serial.println("unknown");
    }
  }
}
