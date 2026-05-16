#include <Wire.h>

static const uint16_t SCAN_INTERVAL_MS = 1000;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.setWireTimeout(3000, true);
  Serial.println("Uno I2C scanner starting");
}

void loop() {
  static uint32_t lastScan = 0;
  uint32_t now = millis();
  if ((uint32_t)(now - lastScan) < SCAN_INTERVAL_MS) {
    return;
  }
  lastScan = now;

  bool foundAny = false;
  Serial.println("Scanning...");

  for (uint8_t address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();

    if (error == 0) {
      foundAny = true;
      Serial.print("Found device at 0x");
      if (address < 16) {
        Serial.print('0');
      }
      Serial.println(address, HEX);
    } else if (error == 4) {
      Serial.print("Unknown error at 0x");
      if (address < 16) {
        Serial.print('0');
      }
      Serial.println(address, HEX);
    }
  }

  if (!foundAny) {
    Serial.println("No I2C devices found");
  }
}
