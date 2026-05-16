/*
  UART_Printer_UNO.ino
  Reads bytes from hardware Serial and prints ASCII and HEX with a simple checksum and error checks.
  Connect the micro TX -> UNO RX (pin 0), micro RX -> UNO TX (pin 1) if needed.
*/

const unsigned long BAUD = 115200;
unsigned long lastRx = 0;
const int BUF_LEN = 128;
char rxBuf[BUF_LEN];
int rxPos = 0;

void setup() {
  Serial.begin(BAUD);
  pinMode(LED_BUILTIN, OUTPUT);
  delay(200);
  while (Serial.available()) Serial.read();  // flush any bytes that arrived during boot
  Serial.println();
  Serial.println("UART Printer UNO starting");
}

void loop() {
  while (Serial.available()) {
    int v = Serial.read();
    lastRx = millis();
    if (v < 0) continue;

    // store byte in buffer (avoid overflow)
    if (rxPos < BUF_LEN - 1) rxBuf[rxPos++] = (char)v;

    // when we receive a newline, print the accumulated line and a hex dump
    if (v == '\n') {
      rxBuf[rxPos] = '\0';

      // trim trailing CR/LF
      int len = rxPos;
      while (len > 0 && (rxBuf[len-1] == '\r' || rxBuf[len-1] == '\n')) rxBuf[--len] = '\0';

      Serial.print("LINE: ");
      Serial.println(rxBuf);

      Serial.print("HEX: ");
      for (int i = 0; i < len; ++i) {
        uint8_t b = (uint8_t)rxBuf[i];
        if (b < 16) Serial.print('0');
        Serial.print(b, HEX);
        Serial.print(' ');
      }
      Serial.println();

      rxPos = 0;
    }
  }
}
