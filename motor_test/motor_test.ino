// motor_test.ino
// Simple per-motor test for X-drive (Uno)

const int motorDirectionPins[4] = {4, 12, 8, 7};
const int motorSpeedPins[4] = {3, 11, 5, 6};
const int testSpeed = 200; // 0-255

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 4; i++) {
    pinMode(motorDirectionPins[i], OUTPUT);
    pinMode(motorSpeedPins[i], OUTPUT);
    digitalWrite(motorDirectionPins[i], LOW);
    analogWrite(motorSpeedPins[i], 0);
  }
  // Ensure motor driver enable (if used) is asserted
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);

  Serial.println("Motor test start");
  delay(500);
}

void loop() {
  for (int i = 0; i < 4; i++) {
    Serial.print("Motor "); Serial.print(i); Serial.println(" FORWARD");
    digitalWrite(motorDirectionPins[i], HIGH);
    analogWrite(motorSpeedPins[i], testSpeed);
    delay(1000);

    analogWrite(motorSpeedPins[i], 0);
    delay(200);

    Serial.print("Motor "); Serial.print(i); Serial.println(" REVERSE");
    digitalWrite(motorDirectionPins[i], LOW);
    analogWrite(motorSpeedPins[i], testSpeed);
    delay(1000);

    analogWrite(motorSpeedPins[i], 0);
    delay(500);
  }

  Serial.println("Cycle complete");
  delay(2000);
}
