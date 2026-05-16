# ============================================================================
# RoboCup Junior Soccer — Goal Camera (I2C Slave)
# OpenMV MicroPython — Communicates with Arduino via I2C
# Fisheye-corrected (equisolid projection) using OV5640 1/4" sensor
# Lens: 1.7mm (marked), 5MP, 1/2.5" (lens coverage)
# ============================================================================

import sensor, image, time, math, ustruct
from pyb import I2C, LED

# ============================================================================
# ██████  CONFIGURATION — CHANGE THESE  ██████
# ============================================================================

CAMERA_FACING = "forward"
SCORING_GOAL  = "blue"
I2C_SLAVE_ADDR = 0x12

# ============================================================================
# ██████  LENS + SENSOR SPECS (RESEARCH-BASED)  ██████
# ============================================================================

F_MM = 1.7  # lens focal length in mm (from lens marking)

# OV5640 (OpenMV H7 Plus R3) sensor specs:
# - 1/4" optical format
# - Active array: 2592 x 1944
# - Pixel size: 1.4 µm
# Source: OV5640 datasheet and OpenMV docs
SENSOR_PIX_W = 2592
PIXEL_PITCH_UM = 1.4

SENSOR_WIDTH_MM = (SENSOR_PIX_W * PIXEL_PITCH_UM) / 1000.0  # = 3.6288 mm

# ============================================================================
# ██████  DERIVED ROLES  ██████
# ============================================================================

if CAMERA_FACING == "forward":
    PRIMARY_GOAL   = SCORING_GOAL
    SECONDARY_GOAL = "yellow" if SCORING_GOAL == "blue" else "blue"
    ROLE = "ATTACK"
else:
    PRIMARY_GOAL   = "yellow" if SCORING_GOAL == "blue" else "blue"
    SECONDARY_GOAL = SCORING_GOAL
    ROLE = "DEFEND"

print("=" * 50)
print("GOAL CAMERA — I2C SLAVE")
print("  Camera facing : %s" % CAMERA_FACING)
print("  Scoring goal  : %s" % SCORING_GOAL)
print("  Role          : %s" % ROLE)
print("  Primary goal  : %s (locked)" % PRIMARY_GOAL)
print("  Secondary goal: %s" % SECONDARY_GOAL)
print("  I2C address   : 0x%02X" % I2C_SLAVE_ADDR)
print("=" * 50)

# ============================================================================
# ██████  COLOR THRESHOLDS (LAB)  ██████
# ============================================================================

BLUE_THRESHOLDS = [
    (51, 72, 3, 29, -128, -44),
    (33, 70, -1, 26, -67, -42),
    (15, 40, -128, -30,  -70, -40),
]

YELLOW_THRESHOLDS = [
    (65, 79, 2, 23, 7, 127),
    (30, 100, -50, 40,  30, 127),
    (40, 120, -40, 50,  40, 127),
]

# ============================================================================
# ██████  CAMERA SETUP  ██████
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

# ============================================================================
# ██████  LEDS FOR VISUAL FEEDBACK  ██████
# ============================================================================

red_led   = LED(1)
green_led = LED(2)
blue_led  = LED(3)

# ============================================================================
# ██████  I2C SLAVE SETUP  ██████
# ============================================================================

i2c = I2C(2, I2C.SLAVE, addr=I2C_SLAVE_ADDR)
PACKET_SIZE = 12
tx_buf = bytearray(PACKET_SIZE)

# ============================================================================
# ██████  DISTANCE / ANGLE HELPERS  ██████
# ============================================================================

DIST_K = 600

# Focal length in pixels (based on actual sensor size + current image width)
FOCAL_PIXELS = (F_MM / SENSOR_WIDTH_MM) * IMG_W

# Compute full horizontal FOV from equisolid model:
# r = 2f * sin(theta/2)
# theta_max = 2 * asin(r_edge / (2f))
r_edge_px = IMG_W / 2
theta_max_rad = 2 * math.asin((r_edge_px) / (2 * FOCAL_PIXELS))
FOV_DEGREES = math.degrees(2 * theta_max_rad)

def estimate_distance(blob):
    area = blob.area()
    if area <= 0:
        return 999
    dist = DIST_K / math.sqrt(area)
    return min(int(dist), 999)

def estimate_angle(blob):
    """
    Equisolid fisheye projection:
    r = 2f * sin(theta/2)
    => theta = 2 * asin(r / (2f))
    """
    dx = blob.cx() - CENTER_X
    r = dx
    val = r / (2 * FOCAL_PIXELS)
    if val > 1: val = 1
    if val < -1: val = -1
    theta_rad = 2 * math.asin(val)
    angle_deg = math.degrees(theta_rad)
    return angle_deg

# ============================================================================
# ██████  FISHEYE-CORRECTED TICK MARKS  ██████
# ============================================================================

def draw_fov_guides(img):
    TICK_COLOR = (180, 180, 180)
    DEG_TICK_SPACING = 10

    # center line
    img.draw_line(CENTER_X, 0, CENTER_X, IMG_H-1, color=TICK_COLOR, thickness=2)

    half_fov = FOV_DEGREES / 2
    for deg in range(-int(half_fov), int(half_fov)+1, DEG_TICK_SPACING):
        theta_rad = math.radians(deg)
        r = 2 * FOCAL_PIXELS * math.sin(theta_rad / 2)
        x = int(round(CENTER_X + r))
        tick_len = 10 if deg % 20 == 0 or deg == 0 else 6
        img.draw_line(x, CENTER_Y - tick_len, x, CENTER_Y + tick_len,
                      color=TICK_COLOR, thickness=2)
        if deg % 20 == 0 or deg == 0:
            img.draw_string(x-8, CENTER_Y + tick_len + 2,
                            "%d" % deg, color=TICK_COLOR, mono_space=False, scale=1)

# ============================================================================
# ██████  GOAL DETECTION  ██████
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
        rect_score = density * (0.5 + 0.5 * aspect_bonus) * b.pixels()
        if rect_score > best_score:
            best_score = rect_score
            best = b
    return best

# ============================================================================
# ██████  I2C PACKET BUILDER  ██████
# ============================================================================

def build_packet(scoring_blob, defending_blob, fps_val):
    header = 0xAA
    status = 0x00

    if scoring_blob:
        status |= 0x01
        s_angle = int(round(estimate_angle(scoring_blob)))
        s_dist  = estimate_distance(scoring_blob)
    else:
        s_angle = 0
        s_dist  = 0

    if defending_blob:
        status |= 0x02
        d_angle = int(round(estimate_angle(defending_blob)))
        d_dist  = estimate_distance(defending_blob)
    else:
        d_angle = 0
        d_dist  = 0

    fps_byte = min(int(fps_val), 255)

    def put_signed(val):
        return (val >> 8) & 0xFF, val & 0xFF

    sa_hi, sa_lo = put_signed(s_angle)
    sd_hi, sd_lo = put_signed(d_angle)

    tx_buf[0]  = header
    tx_buf[1]  = status
    tx_buf[2]  = sa_hi
    tx_buf[3]  = sa_lo
    tx_buf[4]  = (s_dist  >> 8) & 0xFF
    tx_buf[5]  =  s_dist        & 0xFF
    tx_buf[6]  = sd_hi
    tx_buf[7]  = sd_lo
    tx_buf[8]  = (d_dist  >> 8) & 0xFF
    tx_buf[9]  =  d_dist        & 0xFF
    tx_buf[10] = fps_byte

    chk = 0
    for i in range(11):
        chk ^= tx_buf[i]
    tx_buf[11] = chk

    return tx_buf

# ============================================================================
# ██████  I2C TRANSMIT  ██████
# ============================================================================

def send_i2c_data(buf):
    try:
        i2c.send(buf, timeout=5)
    except OSError:
        pass

# ============================================================================
# ██████  DRAWING HELPERS  ██████
# ============================================================================

BLUE_COLOR   = (50, 100, 255)
YELLOW_COLOR = (255, 255, 50)
WHITE_COLOR  = (255, 255, 255)
GREEN_COLOR  = (0,  255,   0)
RED_COLOR    = (255,  50,  50)

def draw_goal_info(img, blob, label, color, angle, dist):
    img.draw_rectangle(blob.rect(), color=color, thickness=2)
    img.draw_cross(blob.cx(), blob.cy(), color=color, size=10)
    img.draw_string(
        blob.x(), blob.y() - 14,
        "%s %ddeg %dcm" % (label, angle, dist),
        color=color, scale=1
    )

# ============================================================================
# ██████  MAIN LOOP  ██████
# ============================================================================

clock = time.clock()
frame_count = 0

GOAL_CONFIG = {
    "blue":   {"thresholds": BLUE_THRESHOLDS,   "color": BLUE_COLOR,   "label": "BLUE"},
    "yellow": {"thresholds": YELLOW_THRESHOLDS, "color": YELLOW_COLOR, "label": "YLW"},
}
scoring_cfg  = GOAL_CONFIG[SCORING_GOAL]
defending_goal_name = "yellow" if SCORING_GOAL == "blue" else "blue"
defending_cfg = GOAL_CONFIG[defending_goal_name]

print("\nRunning main loop...")
print("  Scoring  → detect %s" % scoring_cfg["label"])
print("  Defending→ detect %s" % defending_cfg["label"])
print("  FOV ≈ %.1f° (computed from sensor+lens)" % FOV_DEGREES)
print("")

while True:
    clock.tick()
    img = sensor.snapshot()
    frame_count += 1

    draw_fov_guides(img)

    scoring_blob  = find_best_goal_blob(img, scoring_cfg["thresholds"])
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
        s_dist  = estimate_distance(scoring_blob)
        draw_goal_info(img, scoring_blob,
                       "SCR:" + scoring_cfg["label"],
                       GREEN_COLOR, s_angle, s_dist)

    if defending_blob:
        d_angle = int(round(estimate_angle(defending_blob)))
        d_dist  = estimate_distance(defending_blob)
        draw_goal_info(img, defending_blob,
                       "DEF:" + defending_cfg["label"],
                       RED_COLOR, d_angle, d_dist)

    fps_val = clock.fps()
    img.draw_string(5, 5,
        "%s | %s | FPS:%d" % (ROLE, CAMERA_FACING.upper(), int(fps_val)),
        color=WHITE_COLOR, scale=1)

    pkt = build_packet(scoring_blob, defending_blob, fps_val)
    send_i2c_data(pkt)

    if frame_count % 30 == 0:
        s_info = "NONE"
        d_info = "NONE"
        if scoring_blob:
            s_info = "%ddeg %dcm" % (
                int(round(estimate_angle(scoring_blob))),
                estimate_distance(scoring_blob))
        if defending_blob:
            d_info = "%ddeg %dcm" % (
                int(round(estimate_angle(defending_blob))),
                estimate_distance(defending_blob))
        print("[%s] SCR(%s)=%s  DEF(%s)=%s  FPS=%.1f" % (
            ROLE,
            scoring_cfg["label"], s_info,
            defending_cfg["label"], d_info,
            fps_val
        ))
