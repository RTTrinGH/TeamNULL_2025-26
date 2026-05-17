/*
 * RoboSoccer Ball Sensor Firmware
 *
 * Arduino Micro:
 * - Reads 12 analog IR sensors
 * - Finds the lowest reading
 * - Sends ONLY the sensor label (A-L) over UART
 * - Prints debug info to USB Serial Monitor
 */

const int numSensors = 12;

int readings[numSensors];

void setup() {

  // USB serial for laptop debugging
  Serial.begin(115200);

  // Hardware UART to Arduino Uno
  Serial1.begin(115200);

  delay(1000);

  Serial.println("Ball Sensor Started");
}

void loop() {

  int lowestReading = 1024;
  byte lowestPin = 255;

  // Read all sensors
  for (int pin = 0; pin < numSensors; pin++) {

    int reading = analogRead(pin);

    // Optional saturation cap
    if (reading > 1000) {
      reading = 1023;
    }

    readings[pin] = reading;

    // Find lowest valid sensor
    if (reading < lowestReading && reading != 1023) {
      lowestReading = reading;
      lowestPin = pin;
    }
  }

  // =====================================
  // SEND ONLY LABEL OVER UART
  // =====================================
  if (lowestPin <= 11) {
    Serial1.write('A' + lowestPin);
  }

  // =====================================
  // DEBUG TO LAPTOP ONLY
  // =====================================
  Serial.print("Lowest Sensor: ");
  Serial.print(lowestPin);
  Serial.print("  Reading: ");
  Serial.println(lowestReading);

  /*
  // Uncomment if you want full sensor dump

  for (int i = 0; i < numSensors; i++) {
    Serial.print("A");
    Serial.print(i);
    Serial.print(": ");
    Serial.print(readings[i]);
    Serial.print("  ");
  }

  Serial.println();
  */

  delay(50);
}