const int sensors[12] = {A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11};

void setup() {
  Serial.begin(115200);
  while (!Serial);
}

void loop() {
  for (int i = 0; i < 12; i++) {
    Serial.print(analogRead(sensors[i]));
    Serial.print('\t');
  }
  Serial.println();
  delay(50);
}
