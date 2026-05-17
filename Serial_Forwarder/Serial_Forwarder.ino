/*
  Serial Forwarder
  - Opens USB `Serial` and hardware `Serial1` at 115200
  - Any byte typed into the Serial Monitor is forwarded to `Serial1`
  - Bytes received on `Serial1` are echoed back to Serial Monitor

  Target: Arduino Micro / Leonardo (board with `Serial1`).
*/

const unsigned long BAUD = 115200;

void setup() {
  Serial.begin(BAUD);
  Serial1.begin(BAUD);
  delay(200);
  Serial.println("Serial Forwarder started (type a letter and press Send)");
}

void loop() {
  // Forward from USB Serial to hardware Serial1
  while (Serial.available() > 0) {
    int c = Serial.read();
    Serial1.write(c);
    Serial.print("Sent: ");
    Serial.write(c);
    Serial.println();
  }

  // Echo any bytes coming from Serial1 back to USB Serial
  while (Serial1.available() > 0) {
    int c = Serial1.read();
    Serial.print("RX: ");
    Serial.write(c);
    Serial.println();
  }
}
