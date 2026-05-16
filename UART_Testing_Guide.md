# UART Communication Testing & Troubleshooting Guide

## Phase 1: Hardware Verification

### Step 1: Verify Physical Connections
Before uploading any code:

1. **Power Check**
   - Verify both boards powered: Uno LED on, Micro LED on (if present)
   - Check voltage: Should be ~5V on both VCC pins
   - Verify GND is common (same voltage reference)

2. **UART Connections**
   - Micro TX (pin 1) → Uno pin 9 (SoftwareSerial RX)
   - Micro GND → Uno GND (use separate wire, not just USB reference)
   - No TX from Uno to Micro (one-way communication)

3. **Cable Quality**
   - Use solid core wire (not stranded) for better connection
   - Keep UART lines < 1 meter if possible
   - Avoid routing near motor power lines

### Step 2: Individual Board Testing

**Test Arduino Micro Alone:**
1. Upload `IR_Seeker_Slave_raw_response.ino` to Micro
2. Connect Micro to computer via USB
3. Open Arduino IDE Serial Monitor (Tools → Serial Monitor)
4. Set baud rate to **115200**
5. Expected output: Binary data (looks like garbage - this is correct!)
   - You'll see random-looking characters
   - Should see consistent patterns every ~20 bytes (one packet cycle)
   - If nothing appears, check USB cable and device manager

**Test Arduino Uno Alone:**
1. First comment out all `irSerial` code (just stubs)
2. Upload modified firmware to Uno
3. Open Serial Monitor (115200 baud)
4. Manually drive each motor to verify setup
5. Verify no serial errors on startup

---

## Phase 2: Basic Connectivity Test

### Step 3: Monitor Uno's Packet Reception

1. **Wire boards together** (Micro TX → Uno pin 9 + GND)
2. **Upload both firmware versions** (Micro first, then Uno)
3. **Open Uno Serial Monitor** (115200 baud)
4. Expected output (every 200ms):
   ```
   Strongest sensor: 255 Value: NO_SIGNAL | All: 0 0 0 0 0 0 0 0 0 0 0 0
   ```
   
   OR with IR source nearby:
   ```
   Strongest sensor: 3 Value: 750 | All: 25 45 120 750 180 30 10 5 2 1 8 15
   ```

5. **If no output**: 
   - Add this debug line to Uno setup(): `Serial.println("Starting Uno...");`
   - If this doesn't print, USB communication issue (check cable)

6. **If output but all zeros**:
   - IR sensors not connected to Micro properly
   - Verify Micro is reading sensors (test `Micro_Raw_IR_Test.ino`)

---

## Phase 3: Sensor Data Validation

### Step 4: Verify Sensor Readings

Test with infrared source (IR LED, infrared ball, or IR remote):

1. **Place IR source near Sensor 0** (front of robot)
2. Serial output should show:
   ```
   Strongest sensor: 0 Value: 850 | All: 850 ...
   ```

3. **Move IR source to each sensor position** (0 → 1 → 2 → ... → 11)
4. **Dominant sensor should match position**
   - If not, check sensor wiring
   - Verify sensor pins match A0-A11 in firmware

5. **Check value range**:
   - No signal: 0-50
   - Weak signal: 50-200
   - Medium: 200-600
   - Strong: 600-1023
   - If always maxed out: sensor overexposed, adjust feedback resistor

### Step 5: Sensor Threshold Tuning

Current threshold: `SENSOR_THRESHOLD = 50`

**If false positives** (motor moving when no ball):
- Increase threshold: `const int SENSOR_THRESHOLD = 100;` or `150`
- Re-upload Uno firmware

**If missing weak signals**:
- Decrease threshold: `const int SENSOR_THRESHOLD = 30;` or `25`
- Re-upload Uno firmware

---

## Phase 4: Motor Response Testing

### Step 6: Verify Motor Commands Map Correctly

1. **Record which direction each sensor should drive**
   - Sensor 0 (front) → straight forward
   - Sensor 2 (right) → strafe right
   - Sensor 5 (back) → straight backward
   - Sensor 8 (left) → strafe left

2. **Place IR source at each sensor, observe motion**:
   ```
   Sensor 0 → Robot drives FORWARD ✓
   Sensor 1 → Robot drives FORWARD + RIGHT ✓
   Sensor 2 → Robot drives RIGHT ✓
   Sensor 3 → Robot drives RIGHT ✓
   ...etc
   ```

3. **If motor response is wrong**:
   - Check if physical sensor layout matches firmware assumptions
   - Edit `followBall()` function to correct mappings
   - OR rotate sensor array in firmware if sensors installed differently

4. **If motors don't respond at all**:
   - Check motor pin assignments (pins 4, 12, 8, 7 for direction)
   - Verify motor power supply (separate from logic power?)
   - Test motors directly: `digitalWrite(4, HIGH); analogWrite(3, 160);`

---

## Phase 5: Performance & Stability

### Step 7: Extended Operation Test

Run robot for 5+ minutes with continuous IR input:

1. **Monitor Serial Output** for anomalies:
   ```
   TIMEOUT: No IR data   ← Connection dropout?
   Strongest sensor: 255 ← Lost signal momentarily
   ```

2. **Observe motor behavior**:
   - Should track ball smoothly
   - No sudden jerks or reversals
   - Responds quickly to direction changes

3. **If glitchy**:
   - **Jitter between sensors**: Increase hysteresis (see Helper Functions)
   - **Timeout errors**: Check UART cable connections
   - **Slow response**: Reduce `delay(5)` in Uno loop to `delay(2)`
   - **Noisy readings**: Add capacitor (100nF) across sensor power

### Step 8: Latency Measurement

Measure end-to-end response time:

1. Place IR source in front
2. Record time
3. Move IR source to right
4. Count milliseconds until robot responds
5. **Acceptable**: <200ms (typical 50-100ms)

If latency is high (>200ms):
- Reduce delay in Micro: change `delay(10)` to `delay(5)`
- Reduce motor response delay by optimizing loop

---

## Troubleshooting Decision Tree

### No Serial Output on Uno

```
↓ Does Uno Serial Monitor show anything at startup?
├─ YES: Serial hardware works
│  ├─ "RoboCup Robot Started - UART Mode" message?
│  └─ YES → Check for "TIMEOUT" messages
│      └─ If timeout: UART connection problem
│  └─ NO → Serial.begin() issue?
│      └─ Try different USB cable, different port
└─ NO: USB connection failed
   ├─ Different USB cable?
   ├─ Uno board working? (upload basic blink test)
   └─ Drivers installed? (check Device Manager)
```

### UART Connection Problems

```
↓ Can you see sensors (0 0 0 0...)?
├─ YES: UART working! But sensors not connected
│  └─ Check Micro analog pins A0-A11
└─ NO: UART problem
   ├─ Wire between Micro TX and Uno pin 9?
   ├─ Common GND connected?
   ├─ Both running 115200 baud?
   └─ Micro actually transmitting data?
      └─ Upload Micro_Raw_IR_Test.ino to Micro (should see data over USB)
```

### Motors Don't Respond

```
↓ Do motor pins show correct direction/speed?
├─ YES: Motor wiring issue
│  ├─ Check direction pins (4, 12, 8, 7) connected?
│  ├─ Check speed pins (3, 11, 5, 6) PWM capable?
│  ├─ Check motor power supply?
│  └─ Test: digitalWrite(4, HIGH); analogWrite(3, 200);
└─ NO: Code issue
   ├─ Sensor reading shows correct value?
   ├─ findStrongestSensor() returns correct index?
   ├─ followBall() called with correct index?
   └─ Add debug to each function with Serial.println()
```

### Sensor Values Wrong

```
↓ Which range are readings?
├─ Always 0: Sensors not connected
│  └─ Check A0-A11 wiring
├─ Always ~1023: Sensor overexposed
│  └─ Might need feedback resistor adjustment
├─ Scattered noise: Normal
│  └─ Increase SENSOR_THRESHOLD
└─ Values make sense: Check sensor→direction mapping
   └─ Edit followBall() switch cases
```

---

## Debug Output Enhancement

For troubleshooting, add this to Uno firmware:

```cpp
void debugPacketReception() {
  Serial.print("Packet Index: ");
  Serial.print(packetIndex);
  Serial.print(" | Buffer (first 8 bytes): ");
  
  for (int i = 0; i < 8 && i < packetIndex; i++) {
    Serial.print(packetBuffer[i]);
    Serial.print(" ");
  }
  Serial.println();
}
```

Call in loop after packet buffer update.

---

## Performance Optimization Checklist

Once communication works, optimize for RoboCup:

- [ ] Remove/comment out debug output in `printSensorDebug()`
- [ ] Reduce loop delay from 5ms to 2ms if responsive enough
- [ ] Tune motor speeds (forwardSpeed, strafeSpeed) for your field
- [ ] Increase SENSOR_THRESHOLD if false positives in bright light
- [ ] Test with ball rolling to verify smooth tracking
- [ ] Verify timeout doesn't trigger during normal play (500ms is conservative)
- [ ] Ensure no USB cables interfere with motor power
- [ ] Apply sensor calibration if some sensors read differently

---

## Field Test Checklist (Before Competition)

- [ ] Extended operation test (10+ minutes continuous)
- [ ] Test with actual competition ball and field lighting
- [ ] Verify all 12 sensors detect ball appropriately
- [ ] Test motor response in all 12 directions
- [ ] Measure response latency with actual ball (should be <100ms)
- [ ] Stress test: rapid ball movement around robot
- [ ] Power cycle test: shutdown and restart multiple times
- [ ] Temperature test: leave running for 20 minutes, check stability
- [ ] Radio/WiFi interference test (if other robots nearby)
