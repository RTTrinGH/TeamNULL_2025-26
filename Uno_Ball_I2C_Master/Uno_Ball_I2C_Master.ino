// Arduino I2C Communication Code
// Written by Q Gallant
// Date: September 2, 2023
// Description: This code communicates with an I2C slave device at address 8 and reads data from it.

#include <Wire.h>  // Include the Wire library for I2C communication

void setup() {
  Wire.begin(); // Initialize I2C communication as a master
  Serial.begin(115200); // Start serial communication at 115200 baud rate
}

void loop() {
  Wire.requestFrom(8, 1); // Request 1 byte of data from the I2C slave device at address 8

  while (Wire.available()) { // While there is data available from the slave device
    int minReadingPin = Wire.read(); // Read a byte from the I2C bus and store it in minReadingPin
    Serial.print("Closest sensor: "); // Print a message to the Serial Monitor
    Serial.println(minReadingPin); // Print the value of the received byte (minReadingPin)
  }

  delay(50); // Wait for 50 milliseconds before making the next I2C request
}
