/*
  uno_micro_ball_strongest_monitor.ino
  Reads the strongest sensor index from an Arduino Micro over UART and prints
  a readable message to the Uno Serial Monitor.

  Wiring:
  - Micro TX -> Uno D2
  - Common GND
*/

const unsigned long usbBaud = 115200;

#include <SoftwareSerial.h>

const unsigned long uartBaud = 115200;
const int uartRxPin = 2;
const int uartTxPin = 3;

SoftwareSerial microUart(uartRxPin, uartTxPin);

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(usbBaud);
  microUart.begin(uartBaud);

  delay(100);

  Serial.println("Uno strongest sensor monitor started");
  Serial.println("Listening on D2 for Micro sensor index bytes");
}

void loop() {
  while (microUart.available()) {
    int value = microUart.read();

    if (value >= 0 && value <= 11) {
      Serial.print("Strongest sensor: Sensor ");
      Serial.println(value);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(20);
      digitalWrite(LED_BUILTIN, LOW);
    } else {
      Serial.print("Invalid byte: ");
      Serial.println(value);
    }
  }
}
