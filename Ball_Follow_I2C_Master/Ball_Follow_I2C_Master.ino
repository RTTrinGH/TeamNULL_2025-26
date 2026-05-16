/*
  Ball_Follow_I2C_Master.ino (UART Version - Updated)
  Arduino Uno Main Robot Controller
  
  Receives IR sensor readings from Arduino Micro over UART (replaces I2C).
  Determines ball direction from strongest sensor.
  Drives omnidirectional motors to follow the ball.
  
  Hardware:
  - 4 motors with direction pins (4, 12, 8, 7) and PWM speed pins (3, 11, 5, 6)
  - SoftwareSerial RX on configurable pin (default: pin 9)
  - Common GND with Micro
  - 115200 baud UART connection
  
  Protocol:
  - Receives 24-byte packets from Micro (12 sensors × 2 bytes each)
  - Reconstructs 10-bit sensor values
  - Identifies strongest sensor
  - Maps sensor direction to motor commands
*/

// Using hardware Serial on pins D0 (RX) / D1 (TX)

// Motor control pins
const int motorDirectionPins[4] = {4, 12, 8, 7};
const int motorSpeedPins[4] = {3, 11, 5, 6};

// Motor speed constants
const int forwardSpeed = 160;
const int strafeSpeed = 160;
const int searchSpeed = 100;

// Timing and packet state
unsigned long lastPacketTime = 0;
const unsigned long PACKET_TIMEOUT = 500;  // ms - consider packet lost if no data for 500ms
const unsigned long MOVE_INTERVAL = 1000;   // ms - update drive command once per second
unsigned long lastMoveCommandTime = 0;

// Which sensor index (0-11) is physically aligned with motor 0 (FL reference).
// Set this to rotate the sensor ring so front/back/left/right line up with your robot.
const byte sensorIndexAtMotor0 = 3;

// Last received strongest sensor index from Micro (0-11), 255 = no signal
byte currentStrongestSensor = 255;

void setup() {
  // Use hardware Serial (pins 0/1) at 9600 baud for Micro UART
  Serial.begin(9600);
  
  // Initialize motor control pins
  for (int i = 0; i < 4; i++) {
    pinMode(motorDirectionPins[i], OUTPUT);
    pinMode(motorSpeedPins[i], OUTPUT);
  }
  
  // Pin 10 is typically the enable pin for some motor shields
  pinMode(10, OUTPUT);
  digitalWrite(10, HIGH);
  
  stopMotors();
  
  Serial.println("RoboCup Robot Started - UART Mode");
  Serial.println("Waiting for IR sensor data...");
}

void loop() {
  // Non-blocking hardware-Serial reading: Micro sends a single-byte strongest sensor index
  if (Serial.available()) {
    int incoming = Serial.read();
    if (incoming >= 0) {
      currentStrongestSensor = (byte)incoming;
      lastPacketTime = millis();
    }
  }

  // Check for packet timeout
  if (millis() - lastPacketTime > PACKET_TIMEOUT) {
    currentStrongestSensor = 255;
  }

  // Only update the motion command once per second.
  if (millis() - lastMoveCommandTime >= MOVE_INTERVAL) {
    lastMoveCommandTime = millis();

    if (currentStrongestSensor == 255) {
      searchForBall();
      Serial.println("TIMEOUT: No IR data");
    } else {
      // Map received sensor index to the front reference using sensorIndexAtMotor0
      byte relativeIndex = (currentStrongestSensor + 12 - sensorIndexAtMotor0) % 12;
      followBall(relativeIndex);
      printSensorDebug(currentStrongestSensor);
    }
  }
  
  // Keep loop responsive
  delay(5);
}

/*
  reconstructSensorValues()
  Converts 24-byte binary packet into 12 16-bit sensor values.
  
  Packet format from Micro:
  [S0_H, S0_L, S1_H, S1_L, ..., S11_H, S11_L]
  
  Each sensor value is reconstructed as:
  value = (highByte << 8) | lowByte
  But high byte only contains 2 bits (bits 8-9 of the 10-bit value)
*/
// Reconstruct and per-sensor scanning removed: Micro sends strongest sensor index directly.

/*
  Motor Control Functions
  
  setMotor(index, forward, speed)
    index:   0-3 for motors 0-3
    forward: true for forward, false for reverse
    speed:   0-255 PWM value
    
  drivePolar(angle, magnitude)
    angle:    0-360 degrees (0=front/motor1, 90=right, 180=back, 270=left)
    magnitude: 0-255 speed scaling factor
    
  This converts polar coordinates to X-drive motor mixing
*/

void setMotor(int index, bool forward, int speed) {
  digitalWrite(motorDirectionPins[index], forward ? HIGH : LOW);
  analogWrite(motorSpeedPins[index], speed);
}

void stopMotors() {
  for (int i = 0; i < 4; i++) {
    analogWrite(motorSpeedPins[i], 0);
  }
}

void drivePolar(float angle, int magnitude) {
  // Convert angle (degrees) and magnitude to forward/strafe components
  // angle: 0° = forward, 90° = right, 180° = back, 270° = left
  float angleRad = angle * PI / 180.0;
  
  // Forward/strafe components using trigonometry
  int forward = (int)(cos(angleRad) * magnitude);
  int strafe = (int)(sin(angleRad) * magnitude);

  // Match the motor diagnostic mapping:
  // - Forward/backward uses all 4 motors in the same direction
  // - Strafe right: motors 0/1 forward, motors 2/3 reverse
  // - Strafe left: motors 0/1 reverse, motors 2/3 forward
  
  // Stop all motors before applying the new command
  stopMotors();

  // Combine forward and strafe into per-motor commands.
  // Motor order: 0=FL, 1=BR, 2=BL, 3=FR
  int motorCommand[4];
  motorCommand[0] = forward + strafe;  // FL
  motorCommand[1] = forward + strafe;  // BR
  motorCommand[2] = forward - strafe;  // BL
  motorCommand[3] = forward - strafe;  // FR

  for (int i = 0; i < 4; i++) {
    int speed = motorCommand[i];
    if (speed == 0) {
      analogWrite(motorSpeedPins[i], 0);
    } else if (speed > 0) {
      if (speed > 255) speed = 255;
      setMotor(i, true, speed);
    } else {
      speed = -speed;
      if (speed > 255) speed = 255;
      setMotor(i, false, speed);
    }
  }
}

void searchForBall() {
  // Slow forward creep while searching for the ball.
  drivePolar(0, 30);
}

/*
  followBall(sensorIndex)
  
  Calculates ball angle from sensor index and drives toward it.
  
  Sensor layout: 12 sensors in circle
  - Sensor 0: front (motor 1 forward direction) = 0°
  - Sensor 3: right = 90°
  - Sensor 6: back = 180°
  - Sensor 9: left = 270°
  
  Uses full magnitude for ball tracking speed.
*/
void followBall(byte sensorIndex) {
  if (sensorIndex == 255) {
    searchForBall();
    return;
  }
  
  // Calculate angle to ball
  // 12 sensors = 360°, so each sensor = 30° apart
  // Sensor 0 = 0° (front), increases clockwise
  float angle = (sensorIndex / 12.0) * 360.0;
  // If the robot is moving away from the ball, invert the travel direction.
  // This makes the chassis drive toward the detected sensor instead of away from it.
  angle += 180.0;
  if (angle >= 360.0) {
    angle -= 360.0;
  }
  
  // Use fixed base magnitude since strength is not transmitted by Micro
  int baseMagnitude = 180;  // Base drive speed
  int magnitude = baseMagnitude;
  
  // Drive toward the calculated angle
  drivePolar(angle, magnitude);
}

/*
  printSensorDebug()
  Optional debug output - comment out for competition performance.
  Shows sensor index and raw values over USB serial.
*/
void printSensorDebug(byte sensorIndex) {
  static unsigned long lastDebugTime = 0;
  
  // Print debug info every 200ms to avoid flooding serial
  if (millis() - lastDebugTime > 200) {
    lastDebugTime = millis();
    
    Serial.print("Strongest sensor (raw): ");
    Serial.println(sensorIndex == 255 ? -1 : sensorIndex);
  }
}
