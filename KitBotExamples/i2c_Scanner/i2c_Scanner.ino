// Arduino I2C Scanner
// Written by QEG
// Date: Feb 14, 2024
// Description: This code scans I2C ports connected to a robot to determine what devices are connected and their addresses.

#include <Wire.h>  // Include the Wire library for I2C communication

void setup() {
  Wire.begin();  // Initialize the I2C communication

  Serial.begin(115200);  // Start serial communication at 9600 baud rate
  while (!Serial) ;  // Wait for the serial port to connect. Needed for Leonardo only

  Serial.println("\nI2C Scanner");  // Print a message to the Serial Monitor
}

void loop() {
  byte error, address;  // Variables to store the error status and I2C address
  int nDevices;  // Variable to store the number of devices found

  Serial.println("Scanning...");  // Indicate that scanning has started

  nDevices = 0;  // Initialize the number of devices found to 0
  for (address = 1; address < 127; address++) {  // Iterate over all possible I2C addresses
    Wire.beginTransmission(address);  // Start I2C transmission to the address
    error = Wire.endTransmission();  // End the transmission and get the status

    if (error == 0) {  // If no error, device is found at this address
      Serial.print("I2C device found at address 0x");  // Print the message
      if (address < 16) Serial.print("0");  // Print an extra 0 for addresses less than 0x10
      Serial.print(address, HEX);  // Print the address in hexadecimal
      Serial.println("  !");

      nDevices++;  // Increment the number of devices found
    } else if (error == 4) {  // If there was an error
      Serial.print("Unknown error at address 0x");  // Print the error message
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);  // Print the address with the error
    }
  }
  if (nDevices == 0) {  // If no devices were found
    Serial.print(micros());  // Print the current time in microseconds
    Serial.println("No I2C devices found\n");  // Print the message
  } else Serial.println("done\n");  // Otherwise, print 'done'

  delay(1000);  // Wait for 1000 milliseconds before next scan
}
