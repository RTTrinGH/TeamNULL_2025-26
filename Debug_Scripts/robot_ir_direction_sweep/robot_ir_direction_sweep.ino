/*
  robot_ir_direction_sweep.ino
  Steps through IR sensor directions 0-11 and drives toward and away from
  each direction for 0.75 seconds.

  Motor layout:
  M0 = front (strafe)
  M1 = left (forward/back)
  M2 = back (strafe)
  M3 = right (forward/back)

  Kinematics: Vy = 150*cos(i*30°), Vx = 150*sin(i*30°)
  Motor commands: M0=+Vx, M1=+Vy, M2=-Vx, M3=+Vy
*/

#define FLIP_FWD    1   // flip to -1 if forward runs backward
#define FLIP_STRAFE 1   // flip to -1 if strafe right runs left
#define FLIP_M2     -1  // flip to -1 if M2 (back strafe) is physically inverted
#define TEST_MODE   false

const int motorDirectionPins[4] = {4, 12, 8, 7};
const int motorSpeedPins[4] = {3, 11, 5, 6};
const unsigned long moveDurationMs = 750;
const unsigned long settlePauseMs = 50;
const unsigned long testMotorDurationMs = 600;
const unsigned long testMotorPauseMs = 2000;
const int testMotorSpeed = 120;
const int baseVectorPower = 150;

// Per-motor max power coefficients. M0 and M2 are slightly boosted.
const float motorPowerCoeff[4] = {0.93f, 1.00f, 0.93f, 1.00f};
const int motorMaxPower[4] = {
  (int)(baseVectorPower * motorPowerCoeff[0] + 0.5f),
  (int)(baseVectorPower * motorPowerCoeff[1] + 0.5f),
  (int)(baseVectorPower * motorPowerCoeff[2] + 0.5f),
  (int)(baseVectorPower * motorPowerCoeff[3] + 0.5f)
};

// Sensor index physically aligned with motor 0.
const byte sensorIndexAtMotor0 = 0;

const byte sensorOrder[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

void setup() {
  Serial.begin(115200);

  for (int i = 0; i < 4; i++) {
    pinMode(motorDirectionPins[i], OUTPUT);
    pinMode(motorSpeedPins[i], OUTPUT);
  }

  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);

  stopMotors();
  if (TEST_MODE) {
    Serial.println("=== MOTOR TEST MODE ===");
    delay(500);
  } else {
    Serial.println("IR direction sweep started");
  }
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

void applySignedMotorCommands(const int motorCommand[4]) {
  for (int i = 0; i < 4; i++) {
    int speed = motorCommand[i];
    if (speed >= 0) {
      digitalWrite(motorDirectionPins[i], HIGH);
      analogWrite(motorSpeedPins[i], speed);
    } else {
      digitalWrite(motorDirectionPins[i], LOW);
      analogWrite(motorSpeedPins[i], -speed);
    }
  }
}

int scaleToMotorPower(float normalized, int maxPower) {
  int command = (int)lround(normalized * maxPower);
  if (command > 255) return 255;
  if (command < -255) return -255;
  return command;
}

void computeTowardMotorCommand(byte relativeIndex, int motorCommand[4]) {
  float angleRad = relativeIndex * (PI / 6.0f);
  float vy = cos(angleRad) * FLIP_FWD;
  float vx = sin(angleRad) * FLIP_STRAFE;

  motorCommand[0] = scaleToMotorPower(vx, motorMaxPower[0]);
  motorCommand[1] = scaleToMotorPower(vy, motorMaxPower[1]);
  motorCommand[2] = scaleToMotorPower((-vx) * FLIP_M2, motorMaxPower[2]);
  motorCommand[3] = scaleToMotorPower(vy, motorMaxPower[3]);
}

void applySensorMotorCommand(byte relativeIndex) {
  int motorCommand[4];
  computeTowardMotorCommand(relativeIndex, motorCommand);

  applySignedMotorCommands(motorCommand);
}

byte normalizeSensorIndex(byte sensorIndex) {
  return (sensorIndex + 12 - sensorIndexAtMotor0) % 12;
}

void driveTowardSensor(byte sensorIndex) {
  byte relativeIndex = normalizeSensorIndex(sensorIndex);
  int motorCommand[4];
  computeTowardMotorCommand(relativeIndex, motorCommand);

  Serial.print("Toward sensor ");
  Serial.print(sensorIndex);
  Serial.print(" -> relative ");
  Serial.print(relativeIndex);
  Serial.print(" -> motor commands [");
  for (int i = 0; i < 4; i++) {
    Serial.print(motorCommand[i]);
    if (i < 3) Serial.print(", ");
  }
  Serial.println("]");

  applySignedMotorCommands(motorCommand);
}

void driveAwayFromSensor(byte sensorIndex) {
  byte relativeIndex = normalizeSensorIndex(sensorIndex);
  int motorCommand[4];
  int towardCommand[4];
  computeTowardMotorCommand(relativeIndex, towardCommand);

  Serial.print("Away from sensor ");
  Serial.print(sensorIndex);
  Serial.print(" -> relative ");
  Serial.print(relativeIndex);
  Serial.print(" -> motor commands [");

  for (int i = 0; i < 4; i++) {
    motorCommand[i] = -towardCommand[i];
    Serial.print(motorCommand[i]);
    if (i < 3) Serial.print(", ");
  }

  Serial.println("]");

  applySignedMotorCommands(motorCommand);
}

void stopAndPause() {
  stopMotors();
  delay(settlePauseMs);
}

void testMotorIndividually() {
  const char* motorNames[4] = {"M0 (front/strafe)", "M1 (left/forward-back)", "M2 (back/strafe)", "M3 (right/forward-back)"};
  const char* motorDirections[4] = {"strafe left", "forward", "strafe right", "backward"};

  for (int i = 0; i < 4; i++) {
    Serial.print("\n=== Testing ");
    Serial.print(motorNames[i]);
    Serial.print(" - should move ");
    Serial.print(motorDirections[i]);
    Serial.println(" ===");

    stopMotors();
    delay(200);

    setMotor(i, true, testMotorSpeed);
    delay(testMotorDurationMs);
    stopMotors();
    delay(testMotorPauseMs);
  }

  Serial.println("\n=== Motor test complete ===");
  Serial.println("If any motor moved the wrong direction, adjust:");
  Serial.println("  #define FLIP_FWD    -1  (if M1/M3 forward/backward were reversed)");
  Serial.println("  #define FLIP_STRAFE -1  (if M0 strafe was reversed)");
  Serial.println("  #define FLIP_M2     -1  (if M2 strafe was reversed)");
  Serial.println("\nThen recompile and upload, or change TEST_MODE to false to run sweep.");

  while (true) {
    delay(1000);
  }
}

void loop() {
  if (TEST_MODE) {
    testMotorIndividually();
  } else {
    for (int i = 0; i < 12; i++) {
      byte sensorIndex = sensorOrder[i];

      driveTowardSensor(sensorIndex);
      delay(moveDurationMs);
      stopAndPause();

      driveAwayFromSensor(sensorIndex);
      delay(moveDurationMs);
      stopAndPause();
    }

    stopAndPause();
    delay(250);
  }
}
