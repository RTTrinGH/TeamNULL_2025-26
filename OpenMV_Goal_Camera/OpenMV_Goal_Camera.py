# ============================================================================
# RoboCup Junior Soccer - OpenMV goal camera (I2C slave)
#
# Upload this file to the OpenMV as main.py.
# Change SCORING_GOAL below to "blue" or "yellow".
#
# Packet sent to the Arduino Uno when requested over I2C:
#   [0]  0xAA
#   [1]  status bit0=scoring seen, bit1=defending seen
#   [2]  scoring angle high byte, signed int16, degrees
#   [3]  scoring angle low byte
#   [4]  scoring distance high byte, unsigned int16
#   [5]  scoring distance low byte
#   [6]  defending angle high byte, signed int16, degrees
#   [7]  defending angle low byte
#   [8]  defending distance high byte, unsigned int16
#   [9]  defending distance low byte
#   [10] fps byte
#   [11] xor checksum over bytes 0-10
# ============================================================================

import sensor, image, time, math
from pyb import I2C, LED

# ============================================================================
# Configuration
# ============================================================================

CAMERA_FACING = "forward"
SCORING_GOAL = "blue"       # Change to "yellow" when that is your scoring goal.
I2C_SLAVE_ADDR = 0x12

# Your wide lens is treated as a 100 degree horizontal fisheye lens. The angle
# conversion still uses the equisolid projection: r = 2f * sin(theta / 2).
CALIBRATED_HORIZONTAL_FOV_DEGREES = 100.0

# Fallback lens/sensor values, used only if CALIBRATED_HORIZONTAL_FOV_DEGREES
# is set to None.
F_MM = 1.7
SENSOR_PIX_W = 2592
PIXEL_PITCH_UM = 1.4
SENSOR_WIDTH_MM = (SENSOR_PIX_W * PIXEL_PITCH_UM) / 1000.0

# ============================================================================
# Derived role labels
# ============================================================================

if CAMERA_FACING == "forward":
    PRIMARY_GOAL = SCORING_GOAL
    SECONDARY_GOAL = "yellow" if SCORING_GOAL == "blue" else "blue"
    ROLE = "ATTACK"
else:
    PRIMARY_GOAL = "yellow" if SCORING_GOAL == "blue" else "blue"
    SECONDARY_GOAL = SCORING_GOAL
    ROLE = "DEFEND"

print("=" * 50)
print("GOAL CAMERA - I2C SLAVE")
print("  Camera facing : %s" % CAMERA_FACING)
print("  Scoring goal  : %s" % SCORING_GOAL)
print("  Role          : %s" % ROLE)
print("  Primary goal  : %s" % PRIMARY_GOAL)
print("  Secondary goal: %s" % SECONDARY_GOAL)
print("  I2C address   : 0x%02X" % I2C_SLAVE_ADDR)
print("=" * 50)

# ============================================================================
# Color thresholds in LAB color space
# ============================================================================

BLUE_THRESHOLDS = [
    (51, 72, 3, 29, -128, -44),
    (33, 70, -1, 26, -67, -42),
    (15, 40, -128, -30, -70, -40),
    (0, 43, -13, 21, -128, -25),
]

YELLOW_THRESHOLDS = [
    (65, 79, 2, 23, 7, 127),
    (30, 100, -50, 40, 30, 127),
    (40, 120, -40, 50, 40, 127),
]

# ============================================================================
# Camera setup
# ============================================================================

sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)
sensor.skip_frames(time=2000)

sensor.set_auto_gain(True)
sensor.set_auto_whitebal(True)
sensor.set_auto_exposure(True)
sensor.skip_frames(time=500)

IMG_W = sensor.width()
IMG_H = sensor.height()
CENTER_X = IMG_W // 2
CENTER_Y = IMG_H // 2

if CALIBRATED_HORIZONTAL_FOV_DEGREES:
    edge_angle_rad = math.radians(CALIBRATED_HORIZONTAL_FOV_DEGREES / 2.0)
    FOCAL_PIXELS = (IMG_W / 2.0) / (2.0 * math.sin(edge_angle_rad / 2.0))
    FOV_DEGREES = CALIBRATED_HORIZONTAL_FOV_DEGREES
else:
    FOCAL_PIXELS = (F_MM / SENSOR_WIDTH_MM) * IMG_W
    r_edge_px = IMG_W / 2.0
    theta_max_rad = 2.0 * math.asin(r_edge_px / (2.0 * FOCAL_PIXELS))
    FOV_DEGREES = math.degrees(2.0 * theta_max_rad)

# ============================================================================
# LEDs and I2C
# ============================================================================

red_led = LED(1)
green_led = LED(2)
blue_led = LED(3)

i2c = I2C(2, I2C.SLAVE, addr=I2C_SLAVE_ADDR)
PACKET_SIZE = 12
tx_buf = bytearray(PACKET_SIZE)

# ============================================================================
# Distance and angle helpers
# ============================================================================

DIST_K = 600

def estimate_distance(blob):
    area = blob.area()
    if area <= 0:
        return 999
    dist = DIST_K / math.sqrt(area)
    return min(int(dist), 999)

def estimate_angle(blob):
    dx = blob.cx() - CENTER_X
    val = dx / (2.0 * FOCAL_PIXELS)
    if val > 1.0:
        val = 1.0
    if val < -1.0:
        val = -1.0
    theta_rad = 2.0 * math.asin(val)
    return math.degrees(theta_rad)

# ============================================================================
# Drawing helpers
# ============================================================================

BLUE_COLOR = (50, 100, 255)
YELLOW_COLOR = (255, 255, 50)
WHITE_COLOR = (255, 255, 255)
GREEN_COLOR = (0, 255, 0)
RED_COLOR = (255, 50, 50)
TICK_COLOR = (180, 180, 180)

def draw_fov_guides(img):
    img.draw_line(CENTER_X, 0, CENTER_X, IMG_H - 1, color=TICK_COLOR, thickness=2)

    half_fov = FOV_DEGREES / 2.0
    for deg in range(-int(half_fov), int(half_fov) + 1, 10):
        theta_rad = math.radians(deg)
        r = 2.0 * FOCAL_PIXELS * math.sin(theta_rad / 2.0)
        x = int(round(CENTER_X + r))
        tick_len = 10 if deg % 20 == 0 or deg == 0 else 6
        img.draw_line(x, CENTER_Y - tick_len, x, CENTER_Y + tick_len,
                      color=TICK_COLOR, thickness=2)
        if deg % 20 == 0 or deg == 0:
            img.draw_string(x - 8, CENTER_Y + tick_len + 2,
                            "%d" % deg, color=TICK_COLOR,
                            mono_space=False, scale=1)

def draw_goal_info(img, blob, label, color, angle, dist):
    img.draw_rectangle(blob.rect(), color=color, thickness=2)
    img.draw_cross(blob.cx(), blob.cy(), color=color, size=10)
    img.draw_string(blob.x(), blob.y() - 14,
                    "%s %ddeg %dcm" % (label, angle, dist),
                    color=color, scale=1)

# ============================================================================
# Goal detection
# ============================================================================

def find_best_goal_blob(img, thresholds):
    blobs = img.find_blobs(
        thresholds,
        pixels_threshold=30,
        area_threshold=40,
        merge=True,
        margin=10
    )
    if not blobs:
        return None

    best = None
    best_score = -1
    for b in blobs:
        if b.h() == 0:
            continue
        aspect = b.w() / b.h()
        density = b.density()
        aspect_bonus = min(aspect, 3.0) / 3.0
        score = density * (0.5 + 0.5 * aspect_bonus) * b.pixels()
        if score > best_score:
            best_score = score
            best = b
    return best

# ============================================================================
# Packet builder
# ============================================================================

def put_i16(buf, index, val):
    if val < 0:
        val += 65536
    buf[index] = (val >> 8) & 0xFF
    buf[index + 1] = val & 0xFF

def put_u16(buf, index, val):
    if val < 0:
        val = 0
    if val > 65535:
        val = 65535
    buf[index] = (val >> 8) & 0xFF
    buf[index + 1] = val & 0xFF

def build_packet(scoring_blob, defending_blob, fps_val):
    status = 0

    tx_buf[0] = 0xAA

    if scoring_blob:
        status |= 0x01
        scoring_angle = int(round(estimate_angle(scoring_blob)))
        scoring_dist = estimate_distance(scoring_blob)
    else:
        scoring_angle = 0
        scoring_dist = 0

    if defending_blob:
        status |= 0x02
        defending_angle = int(round(estimate_angle(defending_blob)))
        defending_dist = estimate_distance(defending_blob)
    else:
        defending_angle = 0
        defending_dist = 0

    tx_buf[1] = status
    put_i16(tx_buf, 2, scoring_angle)
    put_u16(tx_buf, 4, scoring_dist)
    put_i16(tx_buf, 6, defending_angle)
    put_u16(tx_buf, 8, defending_dist)
    tx_buf[10] = min(int(fps_val), 255)

    chk = 0
    for i in range(11):
        chk ^= tx_buf[i]
    tx_buf[11] = chk

    return tx_buf

def send_i2c_data(buf):
    try:
        i2c.send(buf, timeout=5)
    except OSError:
        pass

# ============================================================================
# Main loop
# ============================================================================

clock = time.clock()
frame_count = 0

GOAL_CONFIG = {
    "blue": {"thresholds": BLUE_THRESHOLDS, "color": BLUE_COLOR, "label": "BLUE"},
    "yellow": {"thresholds": YELLOW_THRESHOLDS, "color": YELLOW_COLOR, "label": "YLW"},
}

scoring_cfg = GOAL_CONFIG[SCORING_GOAL]
defending_goal_name = "yellow" if SCORING_GOAL == "blue" else "blue"
defending_cfg = GOAL_CONFIG[defending_goal_name]

print("")
print("Running main loop...")
print("  Scoring goal: %s" % scoring_cfg["label"])
print("  Defending goal: %s" % defending_cfg["label"])
print("  Horizontal FOV: %.1f deg" % FOV_DEGREES)
print("")

while True:
    clock.tick()
    img = sensor.snapshot()
    frame_count += 1

    draw_fov_guides(img)

    scoring_blob = find_best_goal_blob(img, scoring_cfg["thresholds"])
    defending_blob = find_best_goal_blob(img, defending_cfg["thresholds"])

    if scoring_blob:
        green_led.on()
    else:
        green_led.off()

    if defending_blob:
        blue_led.on()
    else:
        blue_led.off()

    if scoring_blob:
        s_angle = int(round(estimate_angle(scoring_blob)))
        s_dist = estimate_distance(scoring_blob)
        draw_goal_info(img, scoring_blob, "SCR:" + scoring_cfg["label"],
                       GREEN_COLOR, s_angle, s_dist)

    if defending_blob:
        d_angle = int(round(estimate_angle(defending_blob)))
        d_dist = estimate_distance(defending_blob)
        draw_goal_info(img, defending_blob, "DEF:" + defending_cfg["label"],
                       RED_COLOR, d_angle, d_dist)

    fps_val = clock.fps()
    img.draw_string(5, 5,
                    "%s | %s | FPS:%d" %
                    (ROLE, CAMERA_FACING.upper(), int(fps_val)),
                    color=WHITE_COLOR, scale=1)

    pkt = build_packet(scoring_blob, defending_blob, fps_val)
    send_i2c_data(pkt)

    if frame_count % 30 == 0:
        s_info = "NONE"
        d_info = "NONE"
        if scoring_blob:
            s_info = "%ddeg %dcm" % (
                int(round(estimate_angle(scoring_blob))),
                estimate_distance(scoring_blob)
            )
        if defending_blob:
            d_info = "%ddeg %dcm" % (
                int(round(estimate_angle(defending_blob))),
                estimate_distance(defending_blob)
            )
        print("[%s] SCR(%s)=%s DEF(%s)=%s FPS=%.1f" % (
            ROLE,
            scoring_cfg["label"], s_info,
            defending_cfg["label"], d_info,
            fps_val
        ))
