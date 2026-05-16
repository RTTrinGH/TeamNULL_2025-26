/*
  uno_ir_strongest_monitor.ino
  Reads the strongest IR sensor index from the Micro over UART and prints it
  to the Serial Monitor.

  Wiring expected:
  - Micro TX -> Uno D9
  - Common GND
*/

const unsigned long usbBaud = 115200;
void setup() {
  // Use hardware Serial (pins D0/D1) for Micro communication at 115200.
  // This is the same Serial used by USB on the Uno; open the Uno Serial Monitor
  // on the Uno's COM port to see received bytes.
  Serial.begin(usbBaud);
  delay(100);

  Serial.println("UNO strongest IR monitor started");
  Serial.println("Waiting for bytes from the Micro...");
}

void loop() {
  while (Serial.available()) {
    int value = Serial.read();
    if (value == 255) {
      Serial.println("Strongest sensor: none");
    } else if (value >= 0 && value <= 11) {
      Serial.print("Strongest sensor: ");
      Serial.println(value);
    }
  }
}
