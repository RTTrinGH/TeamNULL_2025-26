# UART RoboCup Firmware - Quick Reference Card

## Wiring Diagram
```
Arduino Micro          Arduino Uno
┌─────────────┐       ┌─────────────┐
│    TX ─────────────→ │  Pin 9 (RX) │
│             │       │             │
│    GND ────────────→ │  GND        │
│             │       │             │
│    5V  ●    │       │  ●  5V      │
│    GND ●    │       │  ●  GND     │
└─────────────┘       │             │
  IR Sensors          Motor Pins    │
  A0-A11             (3,4,5,6,7,8, │
                      11,12)        │
                └─────────────┘
```

## Upload Sequence
1. **Upload Micro first** → `IR_Seeker_Slave_raw_response.ino`
2. **Upload Uno second** → `Ball_Follow_I2C_Master.ino`
3. Open Uno Serial Monitor → 115200 baud
4. Verify output appears (should see sensor data)

## Configuration Constants

### Arduino Micro (`IR_Seeker_Slave_raw_response.ino`)
```cpp
const int sensors[12] = {A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11};
// Modify if sensors on different pins

delay(10);  // Packet transmission rate control
// Reduce to 5 for faster updates (~200 Hz)
// Increase to 20 for slower updates (~50 Hz) + power saving
```

### Arduino Uno (`Ball_Follow_I2C_Master.ino`)
```cpp
const int SOFT_RX_PIN = 9;  // Change if using different pin
// MUST NOT conflict with motor pins: 3, 4, 5, 6, 7, 8, 11, 12

const int motorDirectionPins[4] = {4, 12, 8, 7};   // Change if wired differently
const int motorSpeedPins[4] = {3, 11, 5, 6};       // Must be PWM capable

const int forwardSpeed = 160;    // 0-255 PWM
const int strafeSpeed = 160;     // 0-255 PWM
const int searchSpeed = 100;     // 0-255 PWM

const int SENSOR_THRESHOLD = 50;  // Increase if noisy, decrease if weak signals
const unsigned long PACKET_TIMEOUT = 500;  // ms without data = search mode
```

## Packet Structure
```
Byte 0-1:   Sensor 0  [High (bits 8-9), Low (bits 0-7)]
Byte 2-3:   Sensor 1  [High, Low]
...
Byte 22-23: Sensor 11 [High, Low]
Total: 24 bytes per packet
Frequency: ~100 packets/sec (one every ~10ms)
```

## Serial Communication
```
Baud Rate:        115200 bits/sec
Micro → Uno:      TX (pin 1) → SoftwareSerial pin 9
Connection Type:  One-way (Uno doesn't send to Micro)
Cable Length:     <1 meter recommended
```

## Sensor→Direction Mapping (12-point compass)
```
         0 (Forward)
      
 11                1
     
10                 2

9                   3

 8                 4
     
 7               5,6
         
       (Backward)
```

| Sensor | Direction | Motors |
|--------|-----------|--------|
| 0, 11 | Forward | Both forward |
| 1 | Forward-Right | Forward + Right strafe |
| 2, 3 | Right | Right strafe only |
| 4 | Back-Right | Backward + Right strafe |
| 5, 6 | Backward | Both backward |
| 7 | Back-Left | Backward + Left strafe |
| 8, 9 | Left | Left strafe only |
| 10 | Forward-Left | Forward + Left strafe |

**Adjust if your physical sensor layout is different!**

## Motor Pin Reference
| Motor | Direction Pin | Speed Pin | Direction PIN Values | Role |
|-------|---|---|---|---|
| 0 | 4 | 3 | HIGH=left, LOW=right | Front-Left |
| 1 | 12 | 11 | HIGH=fwd, LOW=back | Back-Right |
| 2 | 8 | 5 | HIGH=left, LOW=right | Back-Left |
| 3 | 7 | 6 | HIGH=fwd, LOW=back | Front-Right |

## Quick Troubleshooting

| Symptom | Cause | Fix |
|---------|-------|-----|
| All sensor values = 0 | Sensors not connected | Check A0-A11 wiring |
| "TIMEOUT: No IR data" | UART connection lost | Check TX/RX/GND wires |
| Robot doesn't respond to IR | Threshold too high | Reduce `SENSOR_THRESHOLD` |
| False movement when no ball | Threshold too low | Increase `SENSOR_THRESHOLD` |
| Motors don't spin | Pin conflict or power | Verify pins, check motor power |
| Jerky motion | Noise/hysteresis | Use helper function `findStrongestSensorWithHysteresis()` |
| Slow response | Latency | Reduce `delay()` values in Micro/Uno |

## Key Functions

### Arduino Micro
```cpp
readAllSensors()        // Read 12 analog pins (A0-A11)
transmitSensorPacket()  // Send 24-byte binary packet
```

### Arduino Uno
```cpp
reconstructSensorValues()    // Convert 24 bytes → 12 sensor values
findStrongestSensor()        // Find index of maximum sensor reading
followBall(sensorIndex)      // Map sensor to motor command
driveVector(fwd, strafe)     // Drive motors given vectors
stopMotors()                 // Stop all motors
searchForBall()              // Rotation search pattern
```

## Debug Serial Output (Uno)
```cpp
// Every 200ms you'll see:
Strongest sensor: 3 Value: 850 | All: 25 45 120 850 180 30 10 5 2 1 8 15

// If no signal:
Strongest sensor: 255 Value: NO_SIGNAL | All: 0 0 0 0 0 0 0 0 0 0 0 0

// If timeout:
TIMEOUT: No IR data
```

## Common Adjustments for Competition

### Fast/Responsive Setup
```cpp
forwardSpeed = 200;
strafeSpeed = 200;
searchSpeed = 130;
delay(5);  // In Micro
delay(2);  // In Uno
SENSOR_THRESHOLD = 40;
```

### Smooth/Stable Setup (Bright Field)
```cpp
forwardSpeed = 140;
strafeSpeed = 140;
searchSpeed = 80;
delay(15);  // In Micro
delay(5);   // In Uno
SENSOR_THRESHOLD = 80;
```

### Power Saving Setup
```cpp
forwardSpeed = 150;
strafeSpeed = 150;
searchSpeed = 100;
delay(20);  // In Micro - slower updates
SENSOR_THRESHOLD = 60;
// Uses ~30% less power
```

## Safety Checklist Before Match
- [ ] Both boards powered and responding
- [ ] Firmware uploaded and verified
- [ ] IR sensors detecting ball
- [ ] Motors responding in all 12 directions
- [ ] Response latency <150ms
- [ ] No timeout errors for 2+ minutes
- [ ] Robot tracks ball smoothly around full circle
- [ ] Debug output disabled (comment out `printSensorDebug()`)
- [ ] All connections secure and not loose
- [ ] Robot battery at full charge

## Files Modified
- `IR_Seeker_Slave_raw_response.ino` → New UART transmitter
- `Ball_Follow_I2C_Master.ino` → New UART receiver + motor control
- `UART_Implementation_Guide.md` → Complete protocol documentation
- `UART_Helper_Functions.cpp` → Optional advanced functions
- `UART_Testing_Guide.md` → Comprehensive troubleshooting

## Support
If issues arise:
1. Check `UART_Testing_Guide.md` decision tree
2. Enable debug output with `printSensorDebug()` call
3. Use `dumpPacketBuffer()` to inspect raw data
4. Verify wiring matches diagram above
5. Test each sensor position individually
