const int numSensors = 12;

int readings[numSensors];
int rawReadings[numSensors];

void setup() {
  // UART serial communication
  Serial.begin(115200);
}

void loop() {
  int lowestReading = 1024;
  int lowestPin = 255;

  for (int pin = 0; pin < numSensors; pin++) {

    int raw = analogRead(pin);

    rawReadings[pin] = raw;

    readings[pin] = (raw > 1000) ? 1023 : raw;

    if (readings[pin] < lowestReading && readings[pin] != 1023) {
      lowestReading = readings[pin];
      lowestPin = pin;
    }
  }

  Serial.print("LOWEST:");
  Serial.print(lowestPin);
  Serial.print(",VALUE:");
  Serial.println(lowestReading);

  for (int i = 0; i < numSensors; i++) {

    Serial.print("A");
    Serial.print(i);

    Serial.print(",RAW:");
    Serial.print(rawReadings[i]);

    Serial.print(",FILTERED:");
    Serial.print(readings[i]);

    Serial.print("  ");
  }

  Serial.println();
  Serial.println("----------------------");

  delay(100);
}
