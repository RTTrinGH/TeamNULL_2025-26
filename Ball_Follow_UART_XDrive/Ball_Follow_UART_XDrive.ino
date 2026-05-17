/*
  Ball_Follow_UART_XDrive.ino
  Arduino Uno ball follower for the Micro UART sensor labels A-L.

  Wiring:
  - Micro TX -> Uno D0 (RX)
  - Uno TX  -> Micro D1 (RX) if you want the full UART pair wired
  - Common GND 

  Behavior:
  - Reads a single label from the Micro: A through L
  - Looks that label up in the A-L motor command dictionary
  - Applies the matching signed motor speeds to move toward the ball
  - Uses the max motor power values from robot_ir_direction_sweep.ino

  Notes:
  - Positive motor values set the direction pin HIGH.
  - Negative motor values set the direction pin LOW.
  - A is treated as sensor 0/front, then B-L step around the ring by 30 degrees.
  - The drive table is rotated 45 degrees counterclockwise from the sensor labels.
  - Change DRIVE_SPEED_SCALE to make every movement faster or slower.
*/

const unsigned long BAUD = 115200;
const unsigned long PACKET_TIMEOUT_MS = 100;
const unsigned long MOVE_INTERVAL_MS = 50;
const unsigned long LED_PULSE_MS = 25;

const int motorDirectionPins[4] = {4, 12, 8, 7};
const int motorSpeedPins[4] = {3, 11, 5, 6};

const float DRIVE_SPEED_SCALE = 0.50f; // 0.50 = half speed, 1.00 = full table speed
const float motorPowerCoeff[4] = {0.93f, 1.00f, 0.93f, 1.00f};
const int baseVectorPower = 150;
const int motorMaxPower[4] = {
  (int)(baseVectorPower * motorPowerCoeff[0] + 0.5f),
  (int)(baseVectorPower * motorPowerCoeff[1] + 0.5f),
  (int)(baseVectorPower * motorPowerCoeff[2] + 0.5f),
  (int)(baseVectorPower * motorPowerCoeff[3] + 0.5f)
};

const bool DEBUG_SERIAL = false;
const byte NO_SENSOR = 255;

struct SensorDriveCommand {
  char label;
  float motorScalar[4];
};

/*
  Direction dictionary:
  - Each value is a scalar from -1.0 to 1.0.
  - Final PWM = scalar * motorMaxPower[motor] * DRIVE_SPEED_SCALE.
  - motorMaxPower is [140, 150, 140, 150] when baseVectorPower is 150.
  - Rotated 45 degrees counterclockwise: command angle = sensor angle - 45 degrees.
*/
const SensorDriveCommand sensorDriveCommands[12] = {
  {'A', {-0.707f,  0.707f, -0.707f,  0.707f}}, // front label, drive 45 deg CCW
  {'B', {-0.259f,  0.966f, -0.259f,  0.966f}},
  {'C', { 0.259f,  0.966f,  0.259f,  0.966f}},
  {'D', { 0.707f,  0.707f,  0.707f,  0.707f}},
  {'E', { 0.966f,  0.259f,  0.966f,  0.259f}},
  {'F', { 0.966f, -0.259f,  0.966f, -0.259f}},
  {'G', { 0.707f, -0.707f,  0.707f, -0.707f}}, // back label
  {'H', { 0.259f, -0.966f,  0.259f, -0.966f}},
  {'I', {-0.259f, -0.966f, -0.259f, -0.966f}},
  {'J', {-0.707f, -0.707f, -0.707f, -0.707f}},
  {'K', {-0.966f, -0.259f, -0.966f, -0.259f}},
  {'L', {-0.966f,  0.259f, -0.966f,  0.259f}}
};

unsigned long lastSensorTime = 0;
unsigned long lastMoveTime = 0;
unsigned long lastLedPulseTime = 0;
byte currentDriveCommandIndex = NO_SENSOR;

char normalizeSensorLabel(char label) {
  if (label >= 'a' && label <= 'l') {
    return label - ('a' - 'A');
  }
  return label;
}

void stopMotors() {
  for (int i = 0; i < 4; i++) {
    analogWrite(motorSpeedPins[i], 0);
  }
}

int scaledMotorCommand(float scalar, int maxPower) {
  float scaled = scalar * maxPower * DRIVE_SPEED_SCALE;
  int command;

  if (scaled >= 0.0f) {
    command = (int)(scaled + 0.5f);
  } else {
    command = (int)(scaled - 0.5f);
  }

  if (command > 255) return 255;
  if (command < -255) return -255;
  return command;
}

void applySignedMotorCommands(const int motorCommand[4]) {
  for (int i = 0; i < 4; i++) {
    int speed = motorCommand[i];
    if (speed > 255) speed = 255;
    if (speed < -255) speed = -255;

    if (speed == 0) {
      analogWrite(motorSpeedPins[i], 0);
    } else if (speed > 0) {
      digitalWrite(motorDirectionPins[i], HIGH);
      analogWrite(motorSpeedPins[i], speed);
    } else {
      digitalWrite(motorDirectionPins[i], LOW);
      analogWrite(motorSpeedPins[i], -speed);
    }
  }
}

int findDriveCommandIndex(char label) {
  label = normalizeSensorLabel(label);
  for (int i = 0; i < 12; i++) {
    if (sensorDriveCommands[i].label == label) {
      return i;
    }
  }
  return -1;
}

void driveTowardCommand(byte commandIndex) {
  if (commandIndex >= 12) {
    stopMotors();
    return;
  }

  int motorCommand[4];
  for (int i = 0; i < 4; i++) {
    motorCommand[i] = scaledMotorCommand(
      sensorDriveCommands[commandIndex].motorScalar[i],
      motorMaxPower[i]
    );
  }

  applySignedMotorCommands(motorCommand);
}

void readSensorLabels() {
  while (Serial.available() > 0) {
    int incoming = Serial.read();
    if (incoming < 0) {
      return;
    }

    char label = (char)incoming;
    if (label == '\r' || label == '\n') {
      continue;
    }

    int commandIndex = findDriveCommandIndex(label);
    if (commandIndex >= 0) {
      currentDriveCommandIndex = (byte)commandIndex;
      lastSensorTime = millis();
      digitalWrite(LED_BUILTIN, HIGH);
      lastLedPulseTime = millis();
    }
  }
}

void setup() {
  Serial.begin(BAUD);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  for (int i = 0; i < 4; i++) {
    pinMode(motorDirectionPins[i], OUTPUT);
    pinMode(motorSpeedPins[i], OUTPUT);
  }

  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);

  stopMotors();
  if (DEBUG_SERIAL) {
    Serial.println("Ball Follow UART X-Drive started");
  }
}

void loop() {
  readSensorLabels();

  if (digitalRead(LED_BUILTIN) == HIGH && millis() - lastLedPulseTime > LED_PULSE_MS) {
    digitalWrite(LED_BUILTIN, LOW);
  }

  if (millis() - lastSensorTime > PACKET_TIMEOUT_MS) {
    currentDriveCommandIndex = NO_SENSOR;
  }

  if (millis() - lastMoveTime >= MOVE_INTERVAL_MS) {
    lastMoveTime = millis();

    if (currentDriveCommandIndex == NO_SENSOR) {
      stopMotors();
    } else {
      driveTowardCommand(currentDriveCommandIndex);
    }
  }
}
