/*
  Game_Ball_Goal_XDrive.ino

  Arduino Uno main game controller:
  - Reads ball direction labels A-L from the Arduino Micro IR board over UART.
  - Reads scoring/defending goal angle and distance from the OpenMV over I2C.
  - Uses a simple state machine:
      FIND_BALL -> CHASE_BALL -> AIM_GOAL -> APPROACH_GOAL
      -> BACKUP_FOR_SHOT -> SHOOT_PUSH -> RECOVER

  Upload targets:
  - This file: Arduino Uno.
  - Debug_Scripts/microBallSensor/microBallSensor.ino: Arduino Micro.
  - OpenMV_Goal_Camera/OpenMV_Goal_Camera.py: OpenMV, saved as main.py.

  Wiring:
  - Micro TX  -> Uno D0/RX
  - OpenMV SDA -> Uno A4/SDA
  - OpenMV SCL -> Uno A5/SCL
  - Common GND between all boards

  Notes:
  - The OpenMV decides which color is the scoring goal. Change SCORING_GOAL
    in the OpenMV script, not in this Uno sketch.
  - Uno hardware Serial is used for the Micro, so Serial Monitor debugging is
    intentionally avoided while driving.
*/

#include <Wire.h>

// ============================================================================
// Communication
// ============================================================================

const unsigned long BALL_BAUD = 115200;
const unsigned long BALL_TIMEOUT_MS = 120;

const byte OPENMV_I2C_ADDR = 0x12;
const byte GOAL_PACKET_SIZE = 12;
const unsigned long GOAL_READ_INTERVAL_MS = 35;
const unsigned long GOAL_STALE_MS = 250;

// ============================================================================
// Motor setup copied from the working X-drive sketches
// ============================================================================

const int motorDirectionPins[4] = {4, 12, 8, 7};
const int motorSpeedPins[4] = {3, 11, 5, 6};

const float motorPowerCoeff[4] = {0.93f, 1.00f, 0.93f, 1.00f};

// Positive turn means "turn right" to match the camera angle convention.
// If positive goal angles make the robot turn left, change TURN_RIGHT_SIGN to -1.
const int TURN_RIGHT_SIGN = 1;
const int turnRightMotorSign[4] = {1, -1, 1, -1};

// The ball is meant to settle into the front indent between motors 0 and 1.
// The old X-drive ball follower used sensorAngle - 45 degrees; keep that here.
const float BALL_APPROACH_OFFSET_DEG = 45.0f;

// ============================================================================
// Tuning values
// ============================================================================

const int BALL_CHASE_POWER = 90;
const int CHASE_GOAL_TURN_MAX = 24;

const int AIM_FORWARD_POWER = 28;
const int APPROACH_FORWARD_POWER = 92;
const int BACKUP_POWER = 72;
const int SHOOT_POWER = 165;

const int MAX_AIM_STRAFE_POWER = 55;
const int MAX_APPROACH_STRAFE_POWER = 70;
const int MAX_SHOOT_STRAFE_POWER = 50;
const int MAX_GOAL_TURN_POWER = 36;

const float GOAL_STRAFE_GAIN = 1.55f;
const float GOAL_TURN_GAIN = 0.85f;

const int AIM_TOLERANCE_DEG = 6;
const int AIM_BREAK_TOLERANCE_DEG = 12;
const int SHOOT_TOLERANCE_DEG = 8;

// OpenMV distance is based on blob area, so tune this on the real field.
const int GOAL_CLOSE_DISTANCE_CM = 65;
const unsigned long APPROACH_TIMEOUT_MS = 2600;

const unsigned long FRONT_LOCK_MS = 280;
const unsigned long FRONT_HOLD_POSSESSION_MS = 650;
const unsigned long POSSESSION_LOST_GRACE_MS = 420;
const unsigned long POSSESSION_ESCAPE_MS = 260;

const unsigned long AIM_LOCK_MS = 180;
const unsigned long SHOT_BACKUP_MS = 220;
const unsigned long SHOT_PUSH_MS = 620;
const unsigned long RECOVER_MS = 260;

const unsigned long MOVE_INTERVAL_MS = 35;
const unsigned long LED_PULSE_MS = 20;

// ============================================================================
// Runtime state
// ============================================================================

const byte NO_SENSOR = 255;

enum RobotState {
  STATE_FIND_BALL,
  STATE_CHASE_BALL,
  STATE_AIM_GOAL,
  STATE_APPROACH_GOAL,
  STATE_BACKUP_FOR_SHOT,
  STATE_SHOOT_PUSH,
  STATE_RECOVER
};

struct GoalData {
  bool packetFresh;
  bool scoringSeen;
  bool defendingSeen;
  int scoringAngle;
  int defendingAngle;
  unsigned int scoringDist;
  unsigned int defendingDist;
  byte fps;
  unsigned long lastPacketMs;
  unsigned long lastScoringSeenMs;
  int lastScoringAngle;
};

RobotState robotState = STATE_FIND_BALL;
unsigned long stateStartMs = 0;
unsigned long lastMoveMs = 0;
unsigned long lastLedPulseMs = 0;
unsigned long lastGoalReadMs = 0;
unsigned long aimLockStartMs = 0;

byte currentBallIndex = NO_SENSOR;
unsigned long lastBallSeenMs = 0;
unsigned long frontBallSinceMs = 0;
unsigned long lastFrontBallSeenMs = 0;
unsigned long nonFrontBallSinceMs = 0;

GoalData goal;
byte goalBuf[GOAL_PACKET_SIZE];

// ============================================================================
// Small helpers
// ============================================================================

int roundToInt(float value) {
  if (value >= 0.0f) {
    return (int)(value + 0.5f);
  }
  return (int)(value - 0.5f);
}

int clampInt(int value, int low, int high) {
  if (value < low) return low;
  if (value > high) return high;
  return value;
}

int absInt(int value) {
  return value < 0 ? -value : value;
}

bool isGoalFresh(unsigned long now) {
  return goal.scoringSeen && (now - goal.lastPacketMs <= GOAL_STALE_MS);
}

bool isFrontBall(byte index) {
  return index == 0 || index == 1 || index == 11;
}

void clearPossessionTracking() {
  frontBallSinceMs = 0;
  lastFrontBallSeenMs = 0;
  nonFrontBallSinceMs = 0;
}

void setState(RobotState nextState, unsigned long now) {
  robotState = nextState;
  stateStartMs = now;
  aimLockStartMs = 0;
}

// ============================================================================
// Motor control
// ============================================================================

void stopMotors() {
  for (int i = 0; i < 4; i++) {
    analogWrite(motorSpeedPins[i], 0);
  }
}

void applySignedMotorCommands(const int motorCommand[4]) {
  for (int i = 0; i < 4; i++) {
    int speed = clampInt(motorCommand[i], -255, 255);

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

int applyMotorCoeff(int value, int motorIndex) {
  return roundToInt(value * motorPowerCoeff[motorIndex]);
}

void driveComponents(int forwardPower, int strafeRightPower, int turnRightPower) {
  int turn = turnRightPower * TURN_RIGHT_SIGN;

  int motorCommand[4];
  motorCommand[0] = applyMotorCoeff(strafeRightPower, 0) + turn * turnRightMotorSign[0];
  motorCommand[1] = applyMotorCoeff(forwardPower, 1)     + turn * turnRightMotorSign[1];
  motorCommand[2] = applyMotorCoeff(strafeRightPower, 2) + turn * turnRightMotorSign[2];
  motorCommand[3] = applyMotorCoeff(forwardPower, 3)     + turn * turnRightMotorSign[3];

  applySignedMotorCommands(motorCommand);
}

void driveByAngle(float angleDeg, int power, int turnRightPower) {
  float angleRad = angleDeg * PI / 180.0f;
  int forwardPower = roundToInt(cos(angleRad) * power);
  int strafePower = roundToInt(sin(angleRad) * power);
  driveComponents(forwardPower, strafePower, turnRightPower);
}

void driveTowardBall(byte ballIndex, int turnRightPower) {
  float ballAngleDeg = ballIndex * 30.0f;
  float driveAngleDeg = ballAngleDeg - BALL_APPROACH_OFFSET_DEG;
  driveByAngle(driveAngleDeg, BALL_CHASE_POWER, turnRightPower);
}

// ============================================================================
// Ball UART
// ============================================================================

char normalizeSensorLabel(char label) {
  if (label >= 'a' && label <= 'l') {
    return label - ('a' - 'A');
  }
  return label;
}

int labelToBallIndex(char label) {
  label = normalizeSensorLabel(label);
  if (label >= 'A' && label <= 'L') {
    return label - 'A';
  }
  return -1;
}

void readBallSensor(unsigned long now) {
  while (Serial.available() > 0) {
    int incoming = Serial.read();
    if (incoming < 0) {
      return;
    }

    char label = (char)incoming;
    if (label == '\r' || label == '\n') {
      continue;
    }

    int index = labelToBallIndex(label);
    if (index >= 0) {
      currentBallIndex = (byte)index;
      lastBallSeenMs = now;
      digitalWrite(LED_BUILTIN, HIGH);
      lastLedPulseMs = now;
    }
  }

  if (currentBallIndex != NO_SENSOR && now - lastBallSeenMs > BALL_TIMEOUT_MS) {
    currentBallIndex = NO_SENSOR;
  }
}

void updateBallPossessionTracking(unsigned long now) {
  if (currentBallIndex == NO_SENSOR) {
    nonFrontBallSinceMs = 0;
    return;
  }

  if (isFrontBall(currentBallIndex)) {
    if (frontBallSinceMs == 0) {
      frontBallSinceMs = now;
    }
    lastFrontBallSeenMs = now;
    nonFrontBallSinceMs = 0;
  } else {
    frontBallSinceMs = 0;
    if (nonFrontBallSinceMs == 0) {
      nonFrontBallSinceMs = now;
    }
  }
}

bool likelyHasBall(unsigned long now) {
  if (frontBallSinceMs == 0) {
    return false;
  }

  if (now - frontBallSinceMs < FRONT_LOCK_MS) {
    return false;
  }

  if (currentBallIndex == NO_SENSOR) {
    return now - lastFrontBallSeenMs <= POSSESSION_LOST_GRACE_MS;
  }

  return isFrontBall(currentBallIndex) &&
         now - frontBallSinceMs >= FRONT_HOLD_POSSESSION_MS;
}

bool possessionEscaped(unsigned long now) {
  return nonFrontBallSinceMs != 0 &&
         now - nonFrontBallSinceMs >= POSSESSION_ESCAPE_MS;
}

// ============================================================================
// OpenMV I2C goal packet
// ============================================================================

int readSigned16(byte hi, byte lo) {
  return (int16_t)(((unsigned int)hi << 8) | lo);
}

unsigned int readUnsigned16(byte hi, byte lo) {
  return ((unsigned int)hi << 8) | lo;
}

bool parseGoalPacket(const byte *buf, unsigned long now) {
  if (buf[0] != 0xAA) {
    return false;
  }

  byte chk = 0;
  for (int i = 0; i < GOAL_PACKET_SIZE - 1; i++) {
    chk ^= buf[i];
  }
  if (chk != buf[GOAL_PACKET_SIZE - 1]) {
    return false;
  }

  byte status = buf[1];
  goal.packetFresh = true;
  goal.scoringSeen = (status & 0x01) != 0;
  goal.defendingSeen = (status & 0x02) != 0;
  goal.scoringAngle = readSigned16(buf[2], buf[3]);
  goal.scoringDist = readUnsigned16(buf[4], buf[5]);
  goal.defendingAngle = readSigned16(buf[6], buf[7]);
  goal.defendingDist = readUnsigned16(buf[8], buf[9]);
  goal.fps = buf[10];
  goal.lastPacketMs = now;

  if (goal.scoringSeen) {
    goal.lastScoringSeenMs = now;
    goal.lastScoringAngle = goal.scoringAngle;
  }

  return true;
}

void readGoalCamera(unsigned long now) {
  if (now - lastGoalReadMs < GOAL_READ_INTERVAL_MS) {
    return;
  }
  lastGoalReadMs = now;

  byte got = Wire.requestFrom(OPENMV_I2C_ADDR, GOAL_PACKET_SIZE);
  byte index = 0;
  while (Wire.available() > 0) {
    byte value = Wire.read();
    if (index < GOAL_PACKET_SIZE) {
      goalBuf[index] = value;
    }
    index++;
  }

  if (got == GOAL_PACKET_SIZE && index >= GOAL_PACKET_SIZE) {
    parseGoalPacket(goalBuf, now);
  }

  if (now - goal.lastPacketMs > GOAL_STALE_MS) {
    goal.packetFresh = false;
    goal.scoringSeen = false;
    goal.defendingSeen = false;
  }
}

// ============================================================================
// Goal steering
// ============================================================================

int scaledGoalCorrection(int angleDeg, float gain, int maxPower) {
  if (absInt(angleDeg) <= AIM_TOLERANCE_DEG) {
    return 0;
  }
  int power = roundToInt(angleDeg * gain);
  return clampInt(power, -maxPower, maxPower);
}

int goalTurnPower(int angleDeg, int maxPower) {
  return scaledGoalCorrection(angleDeg, GOAL_TURN_GAIN, maxPower);
}

int goalStrafePower(int angleDeg, int maxPower) {
  return scaledGoalCorrection(angleDeg, GOAL_STRAFE_GAIN, maxPower);
}

void searchGoalWhileHoldingBall(unsigned long now) {
  int searchDir = 1;
  if (now - goal.lastScoringSeenMs < 1200 && goal.lastScoringAngle < 0) {
    searchDir = -1;
  }

  driveComponents(0, 0, searchDir * 24);
}

// Placeholder for the later line sensors. Put line override here when ready.
bool lineAvoidanceActive() {
  return false;
}

void runLineAvoidance() {
  stopMotors();
}

// ============================================================================
// Game state machine
// ============================================================================

void runFindBall(unsigned long now) {
  if (currentBallIndex != NO_SENSOR) {
    setState(STATE_CHASE_BALL, now);
    return;
  }

  if (now - lastBallSeenMs > POSSESSION_LOST_GRACE_MS) {
    clearPossessionTracking();
  }
  stopMotors();
}

void runChaseBall(unsigned long now) {
  if (likelyHasBall(now)) {
    setState(STATE_AIM_GOAL, now);
    return;
  }

  if (currentBallIndex == NO_SENSOR) {
    setState(STATE_FIND_BALL, now);
    stopMotors();
    return;
  }

  int turnPower = 0;
  if (isGoalFresh(now)) {
    turnPower = goalTurnPower(goal.scoringAngle, CHASE_GOAL_TURN_MAX);
  }

  driveTowardBall(currentBallIndex, turnPower);
}

void runAimGoal(unsigned long now) {
  if (possessionEscaped(now)) {
    setState(STATE_CHASE_BALL, now);
    return;
  }

  if (!isGoalFresh(now)) {
    searchGoalWhileHoldingBall(now);
    return;
  }

  int angle = goal.scoringAngle;
  int absAngle = absInt(angle);
  int strafePower = goalStrafePower(angle, MAX_AIM_STRAFE_POWER);
  int turnPower = goalTurnPower(angle, MAX_GOAL_TURN_POWER);

  if (absAngle <= AIM_TOLERANCE_DEG) {
    if (aimLockStartMs == 0) {
      aimLockStartMs = now;
    }
    driveComponents(AIM_FORWARD_POWER, 0, 0);

    if (now - aimLockStartMs >= AIM_LOCK_MS) {
      setState(STATE_APPROACH_GOAL, now);
    }
    return;
  }

  aimLockStartMs = 0;
  driveComponents(AIM_FORWARD_POWER, strafePower, turnPower);
}

void runApproachGoal(unsigned long now) {
  if (possessionEscaped(now)) {
    setState(STATE_CHASE_BALL, now);
    return;
  }

  if (!isGoalFresh(now)) {
    setState(STATE_AIM_GOAL, now);
    return;
  }

  int angle = goal.scoringAngle;
  if (absInt(angle) > AIM_BREAK_TOLERANCE_DEG) {
    setState(STATE_AIM_GOAL, now);
    return;
  }

  bool closeEnough = goal.scoringDist > 0 &&
                     goal.scoringDist <= GOAL_CLOSE_DISTANCE_CM;
  bool timedOutAligned = now - stateStartMs >= APPROACH_TIMEOUT_MS &&
                         absInt(angle) <= SHOOT_TOLERANCE_DEG;

  if (closeEnough || timedOutAligned) {
    setState(STATE_BACKUP_FOR_SHOT, now);
    return;
  }

  int strafePower = goalStrafePower(angle, MAX_APPROACH_STRAFE_POWER);
  int turnPower = goalTurnPower(angle, MAX_GOAL_TURN_POWER);
  driveComponents(APPROACH_FORWARD_POWER, strafePower, turnPower);
}

void runBackupForShot(unsigned long now) {
  if (now - stateStartMs < SHOT_BACKUP_MS) {
    driveComponents(-BACKUP_POWER, 0, 0);
    return;
  }

  if (isGoalFresh(now) && absInt(goal.scoringAngle) > AIM_BREAK_TOLERANCE_DEG) {
    setState(STATE_AIM_GOAL, now);
    return;
  }

  setState(STATE_SHOOT_PUSH, now);
}

void runShootPush(unsigned long now) {
  int strafePower = 0;
  int turnPower = 0;

  if (isGoalFresh(now)) {
    strafePower = goalStrafePower(goal.scoringAngle, MAX_SHOOT_STRAFE_POWER);
    turnPower = goalTurnPower(goal.scoringAngle, MAX_GOAL_TURN_POWER);
  }

  driveComponents(SHOOT_POWER, strafePower, turnPower);

  if (now - stateStartMs >= SHOT_PUSH_MS) {
    setState(STATE_RECOVER, now);
  }
}

void runRecover(unsigned long now) {
  stopMotors();

  if (now - stateStartMs >= RECOVER_MS) {
    clearPossessionTracking();
    setState(STATE_FIND_BALL, now);
  }
}

void runGameState(unsigned long now) {
  if (lineAvoidanceActive()) {
    runLineAvoidance();
    return;
  }

  switch (robotState) {
    case STATE_FIND_BALL:
      runFindBall(now);
      break;

    case STATE_CHASE_BALL:
      runChaseBall(now);
      break;

    case STATE_AIM_GOAL:
      runAimGoal(now);
      break;

    case STATE_APPROACH_GOAL:
      runApproachGoal(now);
      break;

    case STATE_BACKUP_FOR_SHOT:
      runBackupForShot(now);
      break;

    case STATE_SHOOT_PUSH:
      runShootPush(now);
      break;

    case STATE_RECOVER:
    default:
      runRecover(now);
      break;
  }
}

// ============================================================================
// Arduino entry points
// ============================================================================

void setup() {
  Serial.begin(BALL_BAUD);
  Wire.begin();
#if defined(WIRE_HAS_TIMEOUT)
  Wire.setWireTimeout(25000, true);
#endif

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  for (int i = 0; i < 4; i++) {
    pinMode(motorDirectionPins[i], OUTPUT);
    pinMode(motorSpeedPins[i], OUTPUT);
  }

  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);

  goal.packetFresh = false;
  goal.scoringSeen = false;
  goal.defendingSeen = false;
  goal.lastPacketMs = 0;
  goal.lastScoringSeenMs = 0;
  goal.lastScoringAngle = 0;

  stateStartMs = millis();
  stopMotors();
}

void loop() {
  unsigned long now = millis();

  readBallSensor(now);
  readGoalCamera(now);
  updateBallPossessionTracking(now);

  if (digitalRead(LED_BUILTIN) == HIGH && now - lastLedPulseMs > LED_PULSE_MS) {
    digitalWrite(LED_BUILTIN, LOW);
  }

  if (now - lastMoveMs >= MOVE_INTERVAL_MS) {
    lastMoveMs = now;
    runGameState(now);
  }
}
