/*
 * Author: Q Gallant
 * Date: 2023-10-09
 * Version: 1.0
 * Description: The program is the firmware for teh IT Ball Sensor in the RoboSoccer platform. It is designed for
 * use with an I2C-enabled microcontroller. It reads analog values from multiple sensors, filters the data 
 * through a circular array, identifies the sensor with the lowest value, and sends the index of this sensor over I2C when requested.
*/

#include <Wire.h> // Include the Wire library to enable I2C communication

const int numSensors = 12; // Constant to hold the number of sensors

int readings[numSensors];  // Array to store the readings of each sensor

void setup() {
  // Initialize I2C communication as a slave with address 8
  Wire.begin(8);
  
  // Set the function to be called when data is requested by the master
  Wire.onRequest(sendLowestSensor);
}

void loop() {
  int lowestReading = 1024;  // Start with a value just above the maximum possible analog reading
  
  // Read from analog pins A0 through A11
  for (int pin = 0; pin < numSensors; pin++) {
    int reading = analogRead(pin); // Read the analog value of the current pin
    
    // Store the reading in the readings array, but store 1023 if the reading is over 1000
    readings[pin] = (reading > 1000) ? 1023 : reading;
    
    // Check for the sensor with the lowest reading, excluding the artificial cap of 1023
    if (readings[pin] < lowestReading && readings[pin] != 1023) {
      lowestReading = readings[pin];
    }
  }
}

void sendLowestSensor() {
  int lowestPin = 255;  // Initialize with an invalid pin value to indicate all sensors are capped
  int lowestReading = 1024;  // Initialize with a value above the max analog reading to find the lowest
  
  // Iterate over the sensor readings to find the lowest one
  for (int pin = 0; pin < numSensors; pin++) {
    // Check if current sensor reading is the lowest and not capped
    if (readings[pin] < lowestReading && readings[pin] != 1023) {
      lowestReading = readings[pin]; // Update the lowest reading
      lowestPin = pin; // Update the lowest pin
    }
  }
  
  // Send the index (pin number) of the sensor with the lowest reading over I2C
  Wire.write(lowestPin);
}
