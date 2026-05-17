/*
  uno_line_sensor_monitor.ino
  Simple line sensor monitor for a HW-006 style digital sensor on D8.
  Prints whether the sensor sees black or not.
*/

const unsigned long usbBaud = 115200;
const int lineSensorPin = 8;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(lineSensorPin, INPUT_PULLUP);

  Serial.begin(usbBaud);
  delay(50);

  Serial.println("Line sensor monitor started");
  Serial.println("Reading D8");
  Serial.println("Assuming LOW = black, HIGH = not black");
}

void loop() {
  int sensorState = digitalRead(lineSensorPin);
  bool seesBlack = (sensorState == LOW);

  Serial.print("D8 raw:");
  Serial.print(sensorState);
  Serial.print(" => ");
  Serial.println(seesBlack ? "BLACK" : "NOT BLACK");

  digitalWrite(LED_BUILTIN, seesBlack ? HIGH : LOW);
  delay(200);
}
