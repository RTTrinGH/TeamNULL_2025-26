/*
  robot_cardinal_test.ino
  Repeats forward, backward, left, and right motion for 0.75 seconds each.
  Uses a two-motor pair for forward/back and a separate two-motor pair for
  left/right strafe.
*/

const int motorDirectionPins[4] = {4, 12, 8, 7};
const int motorSpeedPins[4] = {3, 11, 5, 6};
const int testSpeed = 60;
const unsigned long moveDurationMs = 750;

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < 4; i++) {
    pinMode(motorDirectionPins[i], OUTPUT);
    pinMode(motorSpeedPins[i], OUTPUT);
  }

  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);

  stopMotors();
  Serial.println("Cardinal drive test started");
}

void setMotor(int index, bool forward, int speed) {
  digitalWrite(motorDirectionPins[index], forward ? HIGH : LOW);
  analogWrite(motorSpeedPins[index], speed);
}

void stopMotors() {
  for (int i = 0; i < 4; i++) {
    analogWrite(motorSpeedPins[i], 0);
  }
}

void driveForward(int speed) {
  stopMotors();
  setMotor(1, true, speed);
  setMotor(3, true, speed);
  Serial.println("Forward");
}

void driveBackward(int speed) {
  stopMotors();
  setMotor(1, false, speed);
  setMotor(3, false, speed);
  Serial.println("Backward");
}

void driveLeft(int speed) {
  stopMotors();
  setMotor(0, true, speed);
  setMotor(2, true, speed);
  Serial.println("Left");
}

void driveRight(int speed) {
  stopMotors();
  setMotor(0, false, speed);
  setMotor(2, false, speed);
  Serial.println("Right");
}

void loop() {
  driveForward(testSpeed);
  delay(moveDurationMs);

  driveBackward(testSpeed);
  delay(moveDurationMs);

  driveLeft(testSpeed);
  delay(moveDurationMs);

  driveRight(testSpeed);
  delay(moveDurationMs);
}
