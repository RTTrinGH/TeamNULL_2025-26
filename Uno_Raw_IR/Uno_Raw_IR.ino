// Uno_Raw_IR.ino
// Reads analog inputs A0..A5 and prints the strongest sensor and values

const int sensors[] = {A0, A1, A2, A3, A4, A5};
const int numSensors = sizeof(sensors) / sizeof(sensors[0]);

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("Uno_Raw_IR starting");
}

void loop() {
  int maxVal = -1;
  int maxIdx = -1;
  int vals[numSensors];

  for (int i = 0; i < numSensors; i++) {
    vals[i] = analogRead(sensors[i]);
    if (vals[i] > maxVal) {
      maxVal = vals[i];
      maxIdx = i;
    }
  }

  Serial.print("Strongest: A");
  Serial.print(maxIdx);
  Serial.print(" (");
  Serial.print(maxVal);
  Serial.println(")");

  for (int i = 0; i < numSensors; i++) {
    Serial.print("A");
    Serial.print(i);
    Serial.print(": ");
    Serial.print(vals[i]);
    if (i < numSensors - 1) Serial.print('\t');
  }
  Serial.println();

  delay(100);
}
