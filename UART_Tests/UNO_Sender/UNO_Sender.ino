/* UNO_Sender.ino
   Sends a test marker on hardware Serial (TX pin) every 500ms.
   Wire UNO TX (pin 1) -> Micro RX (pin 0) and GND -> GND.
*/

const unsigned long BAUD = 115200;
const unsigned long INTERVAL_MS = 500;
unsigned long last = 0;
unsigned long counter = 1;

void setup() {
  Serial.begin(BAUD);
  delay(100);
  while (Serial.available()) Serial.read();
  Serial.println("UNO Sender starting");
}

void loop() {
  unsigned long now = millis();
  if (now - last >= INTERVAL_MS) {
    Serial.print("<UNO>");
    Serial.println(counter);
    counter++;
    last = now;
  }
}
