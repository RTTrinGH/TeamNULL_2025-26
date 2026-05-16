/*
  uno_raw_echo.ino
  Simple diagnostic: read bytes from Micro (SoftwareSerial on D9)
  and print numeric + hex values to USB Serial, plus blink onboard LED when data arrives.

  Wiring: Micro TX -> Uno D9, common GND
*/

const unsigned long usbBaud = 115200;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  // Use hardware Serial (D0/D1) at 115200 to match the Micro wiring
  Serial.begin(usbBaud);
  delay(50);
  Serial.println("UNO raw echo started");
  Serial.println("Listening on hardware Serial (D0) at 115200");
}

void loop() {
  if (Serial.available()) {
    int b = Serial.read();
    Serial.print("Recv dec:"); Serial.print(b);
    Serial.print("  hex: 0x");
    if (b < 16) Serial.print('0');
    Serial.println(b, HEX);
    // Blink LED once
    digitalWrite(LED_BUILTIN, HIGH);
    delay(50);
    digitalWrite(LED_BUILTIN, LOW);
  }
}
