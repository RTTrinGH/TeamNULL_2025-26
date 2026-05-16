/*
  UART_Helper_Functions.cpp
  
  Utility functions for UART packet parsing, synchronization, and advanced sensor processing.
  These are optional helper functions that can be integrated into the main firmware
  for enhanced error handling, diagnostics, and packet validation.
  
  Include these functions in the Uno firmware as needed.
*/

#include <Arduino.h>

// ============================================================================
// PACKET SYNCHRONIZATION & VALIDATION
// ============================================================================

/*
  findPacketSync()
  
  Searches for packet synchronization by looking for a valid packet boundary.
  Can recover from byte misalignment by scanning for patterns.
  
  Usage: Call this if `reconstructSensorValues()` produces out-of-range values.
  
  Returns: true if sync found, false if still searching
*/
bool findPacketSync(byte* buffer, int bufferSize) {
  // For binary protocol, we look for two consecutive bytes that form a valid 10-bit value
  // High byte should always be 0x00-0x03 (only 2 bits used)
  // This can detect if we're mid-packet
  
  for (int i = 0; i < bufferSize - 1; i++) {
    byte highByte = buffer[i];
    
    // High byte should only have bits 0-1 set (values 0-3)
    if ((highByte & 0xFC) == 0) {
      // Next byte can be anything (0-255)
      // This looks like a valid packet start
      return true;
    }
  }
  
  return false;
}

/*
  validatePacket()
  
  Checks if all sensor values in a reconstructed packet are within valid range.
  Useful for detecting corruption or misalignment.
  
  Parameters:
    sensorValues[12]: Array of reconstructed 10-bit sensor values
    maxValue: Maximum valid sensor value (usually 1023)
  
  Returns: true if all sensors in valid range, false if any out of bounds
*/
bool validatePacket(uint16_t sensorValues[12], uint16_t maxValue = 1023) {
  for (int i = 0; i < 12; i++) {
    if (sensorValues[i] > maxValue) {
      return false;  // Invalid value detected
    }
  }
  return true;
}

// ============================================================================
// SENSOR SIGNAL PROCESSING
// ============================================================================

/*
  smoothSensorValue()
  
  Implements exponential moving average filter for sensor smoothing.
  Reduces noise while maintaining responsiveness.
  
  Parameters:
    sensorValue: New raw sensor reading
    smoothedValue: Previous smoothed value (passed by reference)
    alpha: Smoothing factor (0.0-1.0, higher = more responsive)
           Recommended: 0.3 for smooth tracking
  
  Usage:
    uint16_t smoothed[12] = {0};
    // In loop:
    smoothSensorValue(rawValue, smoothed[i], 0.3);
*/
void smoothSensorValue(uint16_t rawValue, uint16_t& smoothedValue, float alpha) {
  if (alpha < 0.0) alpha = 0.0;
  if (alpha > 1.0) alpha = 1.0;
  
  smoothedValue = (uint16_t)(alpha * rawValue + (1.0 - alpha) * smoothedValue);
}

/*
  applyHysteresis()
  
  Prevents rapid direction changes due to noise near sensor boundaries.
  Requires new reading to exceed previous by a threshold before switching.
  
  Parameters:
    currentStrongest: Current dominant sensor index
    newCandidate: Index of sensor with highest value
    currentValue: Value of current strongest sensor
    candidateValue: Value of new candidate sensor
    hysteresisMargin: Minimum increase needed to switch (0-255)
  
  Returns: true if we should switch to newCandidate, false to keep current
*/
bool applyHysteresis(byte currentStrongest, byte newCandidate, 
                     uint16_t currentValue, uint16_t candidateValue,
                     uint16_t hysteresisMargin = 30) {
  if (newCandidate == 255) return false;  // No valid candidate
  if (currentStrongest == 255) return true;  // No current selection, take it
  
  // Switch only if new value exceeds current by threshold
  return (candidateValue > (currentValue + hysteresisMargin));
}

/*
  findStrongestSensorWithHysteresis()
  
  Enhanced version of findStrongestSensor() with hysteresis to reduce jitter.
  
  Usage in main loop:
    static byte lastSensor = 255;
    byte sensor = findStrongestSensorWithHysteresis(irSensorValues, lastSensor, 50, 30);
    lastSensor = sensor;
*/
byte findStrongestSensorWithHysteresis(uint16_t sensorValues[12], 
                                        byte lastStrongest = 255,
                                        int threshold = 50,
                                        uint16_t hysteresisMargin = 30) {
  uint16_t maxValue = threshold;
  byte maxIndex = 255;
  
  for (int i = 0; i < 12; i++) {
    if (sensorValues[i] > maxValue) {
      maxValue = sensorValues[i];
      maxIndex = i;
    }
  }
  
  // Apply hysteresis if we have a previous value
  if (lastStrongest != 255 && maxIndex != 255) {
    uint16_t lastValue = sensorValues[lastStrongest];
    if (!applyHysteresis(lastStrongest, maxIndex, lastValue, maxValue, hysteresisMargin)) {
      return lastStrongest;  // Keep previous due to hysteresis
    }
  }
  
  return maxIndex;
}

// ============================================================================
// DIAGNOSTIC & DEBUG FUNCTIONS
// ============================================================================

/*
  dumpPacketBuffer()
  
  Prints raw packet buffer contents as hex for debugging.
  Useful for verifying transmission integrity.
  
  Usage: dumpPacketBuffer(packetBuffer, 24);
*/
void dumpPacketBuffer(byte buffer[24]) {
  Serial.print("RAW PACKET [HEX]: ");
  for (int i = 0; i < 24; i++) {
    if (buffer[i] < 0x10) Serial.print("0");
    Serial.print(buffer[i], HEX);
    if ((i + 1) % 2 == 0) Serial.print(" ");
  }
  Serial.println();
}

/*
  printSensorArray()
  
  Pretty-prints all sensor values in a visual format.
  
  Usage: printSensorArray(irSensorValues);
*/
void printSensorArray(uint16_t sensorValues[12]) {
  Serial.print("Sensors: ");
  for (int i = 0; i < 12; i++) {
    if (i < 10) Serial.print(" ");
    Serial.print(sensorValues[i]);
    Serial.print("  ");
    if ((i + 1) % 4 == 0) Serial.print("| ");
  }
  Serial.println();
}

/*
  printSensorVisualization()
  
  Displays sensors as ASCII bar graph showing relative values.
  Great for real-time tuning!
  
  Usage: printSensorVisualization(irSensorValues);
*/
void printSensorVisualization(uint16_t sensorValues[12]) {
  // Find max value for scaling
  uint16_t maxVal = 0;
  for (int i = 0; i < 12; i++) {
    if (sensorValues[i] > maxVal) maxVal = sensorValues[i];
  }
  
  if (maxVal == 0) maxVal = 1;  // Avoid division by zero
  
  Serial.println();
  for (int i = 0; i < 12; i++) {
    Serial.print("S");
    if (i < 10) Serial.print("0");
    Serial.print(i);
    Serial.print(" [");
    
    // Draw bar (20 characters max)
    int barLength = (sensorValues[i] * 20) / maxVal;
    for (int j = 0; j < 20; j++) {
      Serial.print(j < barLength ? "#" : "-");
    }
    
    Serial.print("] ");
    Serial.println(sensorValues[i]);
  }
  Serial.println();
}

/*
  testMotorDirection()
  
  Utility to test motor response. Spins each motor individually for testing.
  
  Usage: 
    // Spin motor 0 forward at speed 200 for 1 second
    testMotorDirection(motorDirectionPins, motorSpeedPins, 0, true, 200, 1000);
*/
void testMotorDirection(const int dirPins[4], const int speedPins[4],
                       int motorIndex, bool forward, int speed, int durationMs) {
  digitalWrite(dirPins[motorIndex], forward ? HIGH : LOW);
  analogWrite(speedPins[motorIndex], speed);
  delay(durationMs);
  analogWrite(speedPins[motorIndex], 0);  // Stop
}

// ============================================================================
// ADVANCED PACKET PARSING
// ============================================================================

/*
  reconstructSensorValuesWithValidation()
  
  Enhanced reconstruction that validates packet before processing.
  Returns false if validation fails, true on success.
  
  Usage:
    if (packetIndex >= 24) {
      if (reconstructSensorValuesWithValidation()) {
        // Sensor data is valid
      } else {
        Serial.println("Packet validation failed!");
      }
      packetIndex = 0;
    }
*/
bool reconstructSensorValuesWithValidation(byte packetBuffer[24], 
                                           uint16_t irSensorValues[12]) {
  // First, check for valid packet boundaries
  for (int i = 0; i < 24; i += 2) {
    byte highByte = packetBuffer[i];
    
    // High byte should only have bits 0-1 set (0x00-0x03)
    if ((highByte & 0xFC) != 0) {
      Serial.print("Packet corruption at byte ");
      Serial.println(i);
      return false;  // Invalid packet
    }
  }
  
  // Reconstruction
  for (int i = 0; i < 12; i++) {
    byte highByte = packetBuffer[i * 2];
    byte lowByte = packetBuffer[i * 2 + 1];
    irSensorValues[i] = ((highByte & 0x03) << 8) | lowByte;
  }
  
  // Verify all values are in valid range
  if (!validatePacket(irSensorValues)) {
    Serial.println("Packet validation failed: Out of range value");
    return false;
  }
  
  return true;
}

// ============================================================================
// CONFIGURATION HELPERS
// ============================================================================

/*
  struct SensorConfig
  
  Container for sensor calibration data.
  Useful if sensors have different sensitivity levels.
*/
struct SensorConfig {
  int threshold;        // Minimum value to consider valid
  uint16_t maxValue;    // Maximum expected value (usually 1023)
  float calibration[12]; // Per-sensor correction factors (1.0 = no correction)
};

/*
  applySensorCalibration()
  
  Applies per-sensor calibration factors to raw sensor values.
  Useful if some sensors are more or less sensitive than others.
  
  Usage:
    SensorConfig config = {50, 1023, {1.0, 1.1, 0.95, 1.0, ...}};
    applySensorCalibration(irSensorValues, config);
*/
void applySensorCalibration(uint16_t sensorValues[12], 
                           const SensorConfig& config) {
  for (int i = 0; i < 12; i++) {
    uint16_t calibrated = (uint16_t)(sensorValues[i] * config.calibration[i]);
    
    // Clamp to max value
    if (calibrated > config.maxValue) {
      calibrated = config.maxValue;
    }
    
    sensorValues[i] = calibrated;
  }
}
