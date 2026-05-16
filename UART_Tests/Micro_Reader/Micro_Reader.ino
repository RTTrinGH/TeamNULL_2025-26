/* Micro_Reader.ino
   Reads bytes from Serial1 (hardware RX pin) and prints them to USB Serial for debugging.
   Wire UNO TX (pin 1) -> Micro RX (pin 0) and GND -> GND.
*/

const unsigned long BAUD = 115200;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(BAUD);
  Serial1.begin(BAUD);
  delay(100);
  Serial.println("MICRO READER starting");
}

void loop() {
  if (Serial1.available()) {
    int c = Serial1.read();
    Serial.print((char)c);
  }
}
