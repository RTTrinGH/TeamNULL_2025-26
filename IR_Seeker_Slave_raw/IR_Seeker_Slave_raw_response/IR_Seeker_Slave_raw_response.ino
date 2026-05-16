/*
  IR_Seeker_Slave_raw_response.ino (UART Version)
  Arduino Micro IR Sensor Processor
  
  Reads 12 analog IR sensors and transmits readings over UART to the Uno main controller.
  Uses binary protocol for compact, high-speed transmission.
  
  Hardware:
  - 12 analog IR sensors on pins A0-A11
  - TX pin sends to Uno RX via SoftwareSerial at 115200 baud
  - Common GND with Uno
  
  Protocol:
  - Continuously reads all 12 sensors (10-bit values: 0-1023)
  - Sends 24-byte packets  (2 bytes per sensor: high byte, low byte)
  - Packet repeats ~100-200 times per second (5-10ms delay)
  - Binary format: [S0_H, S0_L, S1_H, S1_L, ..., S11_H, S11_L]
*/

// Define analog pins for the 12 IR sensors
const int sensors[12] = {A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11};

// Buffer for sensor readings (10-bit values)
uint16_t sensorReadings[12];

void setup() {
  // Initialize hardware UART (Serial1) for TX to Uno at 115200 baud
  // On Arduino Micro, `Serial` is USB CDC; `Serial1` is the UART on TX/RX pins (D0/D1).
  Serial1.begin(115200);

  // Also open USB Serial for local debugging so we can see what bytes are sent
  Serial.begin(115200);
  // Optional: brief initialization indicator
  delay(100);
}

// Minimum sensor value to consider "ball detected"
const int SENSOR_THRESHOLD = 50;

// Enable detailed sensor prints to USB Serial for debugging
#define DEBUG_PRINT_SENSORS 1

// Set to 1 to send an ASCII debug string over Serial1 in addition to the
// single-byte packet. Useful when the receiver or monitor shows garbled
// characters — ASCII mode helps confirm wiring/baud. Set to 0 for normal
// single-byte operation.
#define ASCII_DEBUG_TX 1

void loop() {
  // Read all 12 analog sensors
  readAllSensors();

  // Determine strongest sensor index (0-11), or 255 if none above threshold
  byte strongest = findStrongestSensor();

#if DEBUG_PRINT_SENSORS
  // Print all 12 raw readings for diagnosis
  Serial.print("Sensors:");
  for (int i = 0; i < 12; i++) {
    Serial.print(' ');
    Serial.print(sensorReadings[i]);
  }
  Serial.println();
  if (strongest == 255) {
    Serial.println("Max: none");
  } else {
    Serial.print("Max idx:"); Serial.print(strongest);
    Serial.print(" val:"); Serial.println(sensorReadings[strongest]);
  }
#endif

  // Send single-byte strongest sensor index to Uno over hardware UART
  Serial1.write(strongest);

  // Optional: also send ASCII debug text so a serial monitor can show readable
  // values even when troubleshooting. This can be turned off for runtime use.
#if ASCII_DEBUG_TX
  Serial1.print("IDX:");
  if (strongest == 255) {
    Serial1.println("none");
  } else {
    Serial1.println(strongest);
  }
#endif

  // Also print to USB Serial for debug (helps confirm Micro is transmitting)
  if (Serial)
  {
    if (strongest == 255) {
      Serial.println("Sending: none (255)");
    } else {
      Serial.print("Sending strongest index: ");
      Serial.println(strongest);
    }
  }

  // Small delay to control update rate (~10ms)
  delay(10);
}

/*
  readAllSensors()
  Reads all 12 analog IR sensors and stores values in sensorReadings array.
  Each value is a 10-bit integer (0-1023).
*/
void readAllSensors() {
  for (int i = 0; i < 12; i++) {
    sensorReadings[i] = analogRead(sensors[i]);
  }
}

/*
  transmitSensorPacket()
  Sends all 12 sensor readings as a 24-byte binary packet.
  
  Packet format:
  Byte 0-1:   Sensor 0 (high byte, low byte)
  Byte 2-3:   Sensor 1 (high byte, low byte)
  ...
  Byte 22-23: Sensor 11 (high byte, low byte)
  
  Each 10-bit value is transmitted as 2 bytes:
  - High byte contains bits 8-9 (plus 6 unused bits set to 0)
  - Low byte contains bits 0-7
*/
// legacy: transmitSensorPacket() removed — now sending only strongest sensor index

// Returns the index (0-11) of the sensor with the highest reading above threshold,
// or 255 if no sensor exceeds the threshold.
byte findStrongestSensor() {
  uint16_t maxValue = SENSOR_THRESHOLD;
  byte maxIndex = 255;

  for (int i = 0; i < 12; i++) {
    if (sensorReadings[i] > maxValue) {
      maxValue = sensorReadings[i];
      maxIndex = i;
    }
  }

  return maxIndex;
}