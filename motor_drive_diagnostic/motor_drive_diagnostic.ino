// motor_drive_diagnostic.ino
// Test specific drive commands to diagnose X-drive mixing issue

const int motorDirectionPins[4] = {4, 12, 8, 7};
const int motorSpeedPins[4] = {3, 11, 5, 6};
const int testSpeed = 150;

int testMode = 0; // 0=forward, 1=strafe right, 2=strafe left, 3=rotate
unsigned long modeChangeTime = 0;
const unsigned long MODE_DURATION = 3000; // 3 seconds per mode

void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 4; i++) {
    pinMode(motorDirectionPins[i], OUTPUT);
    pinMode(motorSpeedPins[i], OUTPUT);
  }
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  
  Serial.println("Motor Drive Diagnostic - 3sec per mode");
  Serial.println("0=Forward, 1=Strafe-Right, 2=Strafe-Left, 3=Rotate");
  modeChangeTime = millis();
}

void setMotor(int index, bool forward, int speed) {
  digitalWrite(motorDirectionPins[index], forward ? HIGH : LOW);
  analogWrite(motorSpeedPins[index], speed);
}

void stopAll() {
  for (int i = 0; i < 4; i++) {
    analogWrite(motorSpeedPins[i], 0);
  }
}

void loop() {
  unsigned long now = millis();
  if (now - modeChangeTime > MODE_DURATION) {
    testMode = (testMode + 1) % 4;
    modeChangeTime = now;
    Serial.print("Mode: "); Serial.println(testMode);
  }

  switch (testMode) {
    case 0: // Pure forward - all motors same direction
      Serial.println("FORWARD - all motors same direction");
      setMotor(0, true, testSpeed);  // FL forward
      setMotor(1, true, testSpeed);  // BR forward
      setMotor(2, true, testSpeed);  // BL forward
      setMotor(3, true, testSpeed);  // FR forward
      break;

    case 1: // Pure strafe right
      Serial.println("STRAFE-RIGHT - FL/BR forward, BL/FR reverse");
      setMotor(0, true, testSpeed);   // FL forward
      setMotor(1, true, testSpeed);   // BR forward
      setMotor(2, false, testSpeed);  // BL reverse
      setMotor(3, false, testSpeed);  // FR reverse
      break;

    case 2: // Pure strafe left
      Serial.println("STRAFE-LEFT - BL/FR forward, FL/BR reverse");
      setMotor(0, false, testSpeed);  // FL reverse
      setMotor(1, false, testSpeed);  // BR reverse
      setMotor(2, true, testSpeed);   // BL forward
      setMotor(3, true, testSpeed);   // FR forward
      break;

    case 3: // Pure rotation
      Serial.println("ROTATE - diagonal motors opposite");
      setMotor(0, true, testSpeed);   // FL forward
      setMotor(1, false, testSpeed);  // BR reverse
      setMotor(2, true, testSpeed);   // BL forward
      setMotor(3, false, testSpeed);  // FR reverse
      break;
  }

  delay(100);
}
