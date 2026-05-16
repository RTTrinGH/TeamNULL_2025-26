/*
  UART_Counter_Micro.ino
  Arduino Micro sends increasing integers over Serial1 every 500 ms.
  On the Micro, Serial is USB; Serial1 is the hardware UART on pins 0(RX) and 1(TX).
  USB Serial is kept active as debug output so we can verify the loop is running.
*/

const unsigned long BAUD = 115200;
const unsigned long INTERVAL_MS = 500;
unsigned long lastMillis = 0;
unsigned long counterVal = 1;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(BAUD);
  Serial1.begin(BAUD);
  delay(200);
  Serial.println("MICRO USB DEBUG START");
  Serial1.println("<READY>");
  Serial.println("MICRO UART STARTED");
}

void loop() {
  unsigned long now = millis();
  if (now - lastMillis >= INTERVAL_MS) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    Serial.print("USB COUNTER: ");
    Serial.println(counterVal);
    Serial1.println(counterVal);
    counterVal++;
    lastMillis = now;
  }
}
