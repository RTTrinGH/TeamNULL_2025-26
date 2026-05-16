/*
  motor_sequence.ino
  X-drive sequence: forward, back, left, right for 0.75s each (loop)
  Reuses pin mappings from Move_four_motors_clockwise_countercc.ino
*/

const int motorDirectionPins[4] = {4, 12, 8, 7};
const int motorSpeedPins[4] = {3, 11, 5, 6};
  
void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 4; i++) {
    pinMode(motorDirectionPins[i], OUTPUT);
    pinMode(motorSpeedPins[i], OUTPUT);
  }
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
}
  
// Helper to set motor i: forward=true -> HIGH, forward=false -> LOW
void setMotor(int i, bool forward, int speed) {
  digitalWrite(motorDirectionPins[i], forward ? HIGH : LOW);
  analogWrite(motorSpeedPins[i], speed);
}

void stopAllNoPrint() {
  for (int i = 0; i < 4; i++) analogWrite(motorSpeedPins[i], 0);
}

void driveForward(int speed) {
  stopAllNoPrint();
  setMotor(1, true, speed);
  setMotor(3, true, speed);
  Serial.println("Forward");
}

void driveBackward(int speed) {
  stopAllNoPrint();
  setMotor(1, false, speed);
  setMotor(3, false, speed);
  Serial.println("Backward");
}

// X-drive strafe: adjust motor order/polarity here if wiring differs.
void turnLeft(int speed) {
  stopAllNoPrint();
  // Two-motor strafe left using one diagonal pair
  setMotor(0, true, speed);
  setMotor(2, true, speed);
  Serial.println("Left (strafe)");
}

void turnRight(int speed) {
  stopAllNoPrint();
  // Two-motor strafe right using the other diagonal pair
  setMotor(0, false, speed);
  setMotor(2, false, speed);
  Serial.println("Right (strafe)");
}

void stopMotors() {
  for (int i = 0; i < 4; i++) analogWrite(motorSpeedPins[i], 0);
  Serial.println("Stop");
}

void loop() {
  static int cyclesDone = 0;
  const int speed = 150; // 0-255

  if (cyclesDone >= 2) {
    stopMotors();
    while (true) {
      delay(1000);
    }
  }

  driveForward(speed);
  delay(750);

  driveBackward(speed);
  delay(750);

  turnLeft(speed);
  delay(750);

  turnRight(speed);
  delay(750);

  cyclesDone++;
  stopMotors();
  delay(200);
}
