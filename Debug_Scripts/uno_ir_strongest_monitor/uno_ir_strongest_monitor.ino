/*
  uno_ir_strongest_monitor.ino
  Reads the strongest IR sensor index from the Micro over UART and prints it
  to the Serial Monitor.

  Wiring expected:
  - Micro TX -> Uno D9
  - Common GND
*/

#include <SoftwareSerial.h>

const unsigned long usbBaud = 115200;
const unsigned long microBaud = 9600;
const byte microRxPin = 9;
const byte microTxPin = 8; // unused, but SoftwareSerial needs a TX pin

SoftwareSerial microSerial(microRxPin, microTxPin);

void setup() {
  Serial.begin(usbBaud);
  microSerial.begin(microBaud);
  delay(100);

  Serial.println("UNO strongest IR monitor started");
  Serial.println("Waiting for bytes from the Micro...");
}

void loop() {
  while (microSerial.available()) {
    int value = microSerial.read();

    if (value == 255) {
      Serial.println("Strongest sensor: none");
    } else if (value >= 0 && value <= 11) {
      Serial.print("Strongest sensor: ");
      Serial.println(value);
    } else {
      Serial.print("Unexpected byte: ");
      Serial.println(value);
    }
  }
}
