import time, math
from machine import UART, FPIOA, Pin
from media.sensor import *
from media.display import *
from media.media import *
import cv_lite

# ========== 1. IO引脚初始化 ==========
fpioa = FPIOA()

# X轴舵机串口（GPIO40 TX）
fpioa.set_function(40, FPIOA.UART1_TXD)

# Y轴舵机串口（GPIO44 TX）使用UART2
fpioa.set_function(44, FPIOA.UART2_TXD)

# 激光发射引脚（GPIO24 输出）
fpioa.set_function(24, FPIOA.GPIO24)
laser_pin = Pin(24, Pin.OUT, pull=Pin.PULL_NONE, drive=7)
laser_pin.value(0)  # 初始关闭激光

# 接收信息的串口（GPIO41 RX）使用UART1_RX
fpioa.set_function(41, FPIOA.UART1_RXD)

# 初始化串口
# UART1: 用于X轴舵机控制 + 接收外部指令（共用同一UART，因为RX/TX分开）
uart_x = UART(UART.UART1, baudrate=115200, bits=UART.EIGHTBITS,
              parity=UART.PARITY_NONE, stop=UART.STOPBITS_ONE)

# UART2: 用于Y轴舵机控制
uart_y = UART(UART.UART2, baudrate=115200, bits=UART.EIGHTBITS,
              parity=UART.PARITY_NONE, stop=UART.STOPBITS_ONE)

def send_cmd_x(cmd):
    """发送X轴舵机指令（通过UART1）"""
    full = "#" + cmd + "!"
    uart_x.write(full)

def send_cmd_y(cmd):
    """发送Y轴舵机指令（通过UART2）"""
    full = "#" + cmd + "!"
    uart_y.write(full)

def servo_move_x(pulse, time_ms=50):
    """控制X轴舵机移动到指定位置（ID=1）"""
    id_str = "255"           # 这里将你自己的x轴舵机id填入，一定是三位数，不足三位的高位补0
    pulse_str = f"{pulse:04d}"
    time_str = f"{time_ms:04d}"
    send_cmd_x(f"{id_str}P{pulse_str}T{time_str}")

def servo_move_y(pulse, time_ms=50):
    """控制Y轴舵机移动到指定位置（ID=2）"""
    id_str = "255"           # 填入y轴id
    pulse_str = f"{pulse:04d}"
    time_str = f"{time_ms:04d}"
    send_cmd_y(f"{id_str}P{pulse_str}T{time_str}")

def laser_on():
    """打开激光"""
    laser_pin.value(1)
    print("激光开启")

def laser_off():
    """关闭激光"""
    laser_pin.value(0)
    print("激光关闭")

# ========== 2. 摄像头初始化 ==========
sensor = Sensor(width=1280, height=960)
sensor.reset()
sensor.set_framesize(width=320, height=240)
sensor.set_pixformat(Sensor.RGB565)

Display.init(Display.VIRT, width=320, height=240, to_ide=True)
MediaManager.init()
sensor.run()

clock = time.clock()
#这里是改激光落点
mid_x = 120
mid_y = 150

# 舵机初始位置（脉宽值，1500=中位）
current_pos_x = 800
current_pos_y = 800

# 控制参数
DEAD_ZONE = 25       # 死区（像素）：判断激光是否与靶面中心重合
STEP_X = 20          # X轴速度
STEP_Y = 20          # Y轴速度

last_move_time_x = 0     # X轴上次发送指令时间
last_move_time_y = 0     # Y轴上次发送指令时间
MOVE_INTERVAL_MS = 10    # 舵机指令最小间隔（毫秒）

# ========== 3. 激光控制相关变量 ==========
LASER_PULSE_ON   = 200      # 每次亮200ms
LASER_PULSE_OFF  = 200      # 每次灭200ms
LASER_PULSE_NUM  = 3        # 对准后打3次
laser_fired      = False    # 本次对准是否已打过
laser_pulse_idx  = 0        # 当前第几次
laser_pulse_state = False   # 激光当前亮/灭
laser_pulse_timer = 0       # 脉冲计时
align_stable_cnt = 0        # 连续对准帧数
ALIGN_STABLE_NEED = 15      # 需连续对准15帧(~1秒)

# ========== 4. 矩形检测参数 ==========
canny_thresh1 = 50
canny_thresh2 = 150
approx_epsilon = 40       # 实际值 = 40/1000 = 0.04
area_min_ratio = 5        # 实际值 = 5/1000 = 0.005
max_angle_cos = 3         # 实际值 = 3/10 = 0.3
gaussian_blur_size = 3

RECT_WIDTH = 210
RECT_HEIGHT = 148

MIN_AREA = 100
MAX_AREA = 100000
MIN_ASPECT_RATIO = 0.3
MAX_ASPECT_RATIO = 3.0

image_shape = [sensor.height(), sensor.width()]  # [240, 320]

# ========== 5. 几何计算函数 ==========
def calculate_distance(p1, p2):
    return math.sqrt((p2[0] - p1[0])**2 + (p2[1] - p1[1])**2)

def calculate_center(points):
    if not points:
        return (0, 0)
    sum_x = sum(p[0] for p in points)
    sum_y = sum(p[1] for p in points)
    return (sum_x / len(points), sum_y / len(points))

def is_valid_rect(corners):
    edges = [calculate_distance(corners[i], corners[(i+1)%4]) for i in range(4)]

    ratio1 = edges[0] / max(edges[2], 0.1)
    ratio2 = edges[1] / max(edges[3], 0.1)
    valid_ratio = 0.5 < ratio1 < 1.5 and 0.5 < ratio2 < 1.5

    area = 0
    for i in range(4):
        x1, y1 = corners[i]
        x2, y2 = corners[(i+1) % 4]
        area += (x1 * y2 - x2 * y1)
    area = abs(area) / 2
    valid_area = MIN_AREA < area < MAX_AREA

    width = max(p[0] for p in corners) - min(p[0] for p in corners)
    height = max(p[1] for p in corners) - min(p[1] for p in corners)
    aspect_ratio = width / max(height, 0.1)
    valid_aspect = MIN_ASPECT_RATIO < aspect_ratio < MAX_ASPECT_RATIO

    return valid_ratio and valid_area and valid_aspect

def sort_corners(corners):
    center = calculate_center(corners)
    sorted_corners = sorted(corners, key=lambda p: math.atan2(p[1]-center[1], p[0]-center[0]))

    if len(sorted_corners) == 4:
        left_top = min(sorted_corners, key=lambda p: p[0]+p[1])
        index = sorted_corners.index(left_top)
        sorted_corners = sorted_corners[index:] + sorted_corners[:index]
    return sorted_corners

def get_perspective_matrix(src_pts, dst_pts):
    A = []
    B = []
    for i in range(4):
        x, y = src_pts[i]
        u, v = dst_pts[i]
        A.append([x, y, 1, 0, 0, 0, -u*x, -u*y])
        A.append([0, 0, 0, x, y, 1, -v*x, -v*y])
        B.append(u)
        B.append(v)

    n = 8
    for i in range(n):
        max_row = i
        for j in range(i, len(A)):
            if abs(A[j][i]) > abs(A[max_row][i]):
                max_row = j
        A[i], A[max_row] = A[max_row], A[i]
        B[i], B[max_row] = B[max_row], B[i]

        pivot = A[i][i]
        if abs(pivot) < 1e-8:
            return None
        for j in range(i, n):
            A[i][j] /= pivot
        B[i] /= pivot

        for j in range(len(A)):
            if j != i and A[j][i] != 0:
                factor = A[j][i]
                for k in range(i, n):
                    A[j][k] -= factor * A[i][k]
                B[j] -= factor * B[i]

    return [[B[0], B[1], B[2]],
            [B[3], B[4], B[5]],
            [B[6], B[7], 1.0]]

def transform_point(x, y, matrix):
    x_hom = x * matrix[0][0] + y * matrix[0][1] + matrix[0][2]
    y_hom = x * matrix[1][0] + y * matrix[1][1] + matrix[1][2]
    w_hom = x * matrix[2][0] + y * matrix[2][1] + matrix[2][2]
    if abs(w_hom) > 1e-8:
        return (int(x_hom / w_hom), int(y_hom / w_hom))
    return (x, y)

def move_to_center(error_x, error_y, detected):
    """
    控制舵机向中心移动
    X轴使用GPIO40（UART1），Y轴使用GPIO44（UART2）
    """
    global current_pos_x, current_pos_y, last_move_time_x, last_move_time_y

    if not detected:
        return False

    current_time = time.ticks_ms()
    aligned = True

    # X轴控制（使用UART1）
    if abs(error_x) > DEAD_ZONE:
        aligned = False
        if (current_time - last_move_time_x) >= MOVE_INTERVAL_MS:
            if error_x > 0:
                current_pos_x -= STEP_X   # 目标在右侧，向右移动
            else:
                current_pos_x += STEP_X   # 目标在左侧，向左移动
            current_pos_x = max(500, min(2500, current_pos_x))
            servo_move_x(current_pos_x, 40)
            last_move_time_x = current_time

    # Y轴控制（使用UART2）
    if abs(error_y) > DEAD_ZONE:
        aligned = False
        if (current_time - last_move_time_y) >= MOVE_INTERVAL_MS:
            if error_y > 0:
                current_pos_y += STEP_Y   # 目标在下侧，向下移动
            else:
                current_pos_y -= STEP_Y   # 目标在上侧，向上移动
            current_pos_y = max(500, min(2500, current_pos_y))
            servo_move_y(current_pos_y, 40)
            last_move_time_y = current_time

    return aligned

# ========== 6. 靶面检测函数 ==========
def detect_target_by_rect(img):
    """使用cv_lite检测矩形，通过透视变换定位靶心"""
    gray = img.to_grayscale()
    img_np = gray.to_numpy_ref()

    rects = cv_lite.grayscale_find_rectangles_with_corners(
        image_shape,
        img_np,
        canny_thresh1,
        canny_thresh2,
        approx_epsilon,
        area_min_ratio,
        max_angle_cos,
        gaussian_blur_size
    )

    if not rects:
        return None, False

    min_area = float('inf')
    best_corners = None

    for rect in rects:
        x, y, w, h = rect[0], rect[1], rect[2], rect[3]
        corners = [
            (rect[4], rect[5]),
            (rect[6], rect[7]),
            (rect[8], rect[9]),
            (rect[10], rect[11])
        ]

        if is_valid_rect(corners):
            area = w * h
            if area < min_area:
                min_area = area
                best_corners = corners

    if not best_corners:
        return None, False

    sorted_corners = sort_corners(best_corners)

    # 绘制矩形边框
    for i in range(4):
        x1, y1 = sorted_corners[i]
        x2, y2 = sorted_corners[(i+1) % 4]
        img.draw_line(int(x1), int(y1), int(x2), int(y2), color=(255, 0, 0), thickness=2)

    virtual_rect = [(0, 0), (RECT_WIDTH, 0), (RECT_WIDTH, RECT_HEIGHT), (0, RECT_HEIGHT)]
    matrix = get_perspective_matrix(virtual_rect, sorted_corners)

    if not matrix:
        return None, False

    virtual_center = (RECT_WIDTH / 2, RECT_HEIGHT / 2)
    cx, cy = transform_point(virtual_center[0], virtual_center[1], matrix)

    img.draw_cross(cx, cy, 12, color=(0, 255, 0), thickness=2)
    img.draw_circle(cx, cy, 5, color=(0, 255, 0), thickness=2)

    return (cx, cy), True

def detect_target_hough(img):
    """霍夫圆检测（备用）"""
    gray = img.to_grayscale()
    circles = gray.find_circles(
        threshold=3500,
        r_min=15,
        r_max=60,
        x_scale=2,
        y_scale=2,
        r_step=2
    )

    if circles:
        max_circle = max(circles, key=lambda c: c[2])
        cx, cy, r = max_circle[0], max_circle[1], max_circle[2]
        img.draw_circle(cx, cy, r, color=(255, 255, 0), thickness=2)
        img.draw_cross(cx, cy, 10, color=(255, 255, 0), thickness=2)
        return (cx, cy), True

    return None, False

def detect_target(img):
    """混合检测"""
    target, detected = detect_target_by_rect(img)

    if detected:
        img.draw_string_advanced(5, 45, 16, "Mode:Rect", color=(0, 255, 0))
        return target, detected

    target, detected = detect_target_hough(img)

    if detected:
        img.draw_string_advanced(5, 45, 16, "Mode:Hough", color=(255, 255, 0))
        return target, detected

    img.draw_string_advanced(5, 45, 16, "Mode:None", color=(255, 0, 0))
    return None, False

# ========== 7. 激光控制函数 ==========
# ========== 7. 激光控制函数 ==========
def laser_fire_3_pulses(aligned):
    """对准靶心且稳定后打3次脉冲"""
    global laser_fired, laser_pulse_idx, laser_pulse_state, laser_pulse_timer
    global align_stable_cnt

    now = time.ticks_ms()

    if aligned:
        align_stable_cnt += 1
    else:
        align_stable_cnt = 0
        laser_fired = False
        laser_pulse_idx = 0
        laser_pulse_state = False
        laser_pin.value(0)
        return

    if align_stable_cnt < ALIGN_STABLE_NEED:
        return

    if laser_fired and laser_pulse_idx >= LASER_PULSE_NUM:
        return

    if not laser_fired:
        laser_fired = True
        laser_pulse_idx = 0
        laser_pulse_state = True
        laser_pin.value(1)
        laser_pulse_timer = now
        print("对准！第1次打靶")
        return

    elapsed = now - laser_pulse_timer
    if laser_pulse_state:
        if elapsed >= LASER_PULSE_ON:
            laser_pin.value(0)
            laser_pulse_state = False
            laser_pulse_timer = now
            print("第{}次结束".format(laser_pulse_idx+1))
    else:
        if elapsed >= LASER_PULSE_OFF:
            laser_pulse_idx += 1
            if laser_pulse_idx < LASER_PULSE_NUM:
                laser_pin.value(1)
                laser_pulse_state = True
                laser_pulse_timer = now
                print("第{}次打靶".format(laser_pulse_idx+1))
            else:
                print("3次打靶完成")
print("指令接收: GPIO41 (UART1_RX)")
print("激光控制: GPIO24")
print("分辨率:320x240")
print("=" * 50)

frame_count = 0
last_target = None
target_detected = False
aligned = False
servo_move_x(current_pos_x,40)
servo_move_y(current_pos_y,40)
while True:
    clock.tick()
    img = sensor.snapshot()

    frame_count += 1

    # 1. 读取串口指令（通过GPIO41）
        # auto-fire based on alignment

    # 2. 检测靶面
    target, target_detected = detect_target(img)

    if target_detected and target:
        last_target = target

    # 3. 绘制画面中心
    img.draw_cross(mid_x, mid_y, 10, color=(255, 255, 0), thickness=1)

    # 4. 计算误差和控制舵机
    if target_detected and last_target:
        cx, cy = last_target
        error_x = cx - mid_x
        error_y = cy - mid_y

        img.draw_line(mid_x, mid_y, cx, cy, color=(255, 0, 0), thickness=2)
        img.draw_string_advanced(5, 20, 16, "E_x:{}".format(error_x), color=(255, 0, 0))
        img.draw_string_advanced(5, 35, 16, "E_y:{}".format(error_y), color=(255, 0, 0))

        # 控制舵机移动
        aligned = move_to_center(error_x, error_y, target_detected)

        # 显示对准状态
        if aligned:
            img.draw_string_advanced(5, 80, 16, "ALIGNED", color=(0, 255, 0))
        else:
            img.draw_string_advanced(5, 80, 16, "MOVING...", color=(255, 255, 0))

    elif last_target and not target_detected:
        img.draw_string_advanced(5, 20, 16, "Lost target", color=(255, 0, 0))
        aligned = False
    else:
        img.draw_string_advanced(5, 20, 16, "No target", color=(255, 0, 0))
        aligned = False

    # 5. 激光控制：稳定对准后自动打3次
    laser_fire_3_pulses(aligned)

    # 显示激光状态
    if laser_pulse_state:
        img.draw_string_advanced(5, 95, 16, "LASER:ON", color=(255, 0, 0))
    else:
        img.draw_string_advanced(5, 95, 16, "LASER:OFF", color=(100, 100, 100))
    # 6. 显示FPS
    fps = clock.fps()
    img.draw_string_advanced(5, 5, 16, "FPS:{:.1f}".format(fps), color=(255, 0, 0))

    # 显示舵机位置信息
    img.draw_string_advanced(5, 130, 12, "X:{} Y:{}".format(current_pos_x, current_pos_y), color=(150, 150, 150))

    # 7. 显示图像
    Display.show_image(img)

    # 控制台输出（每30帧输出一次）
    if frame_count % 30 == 0 and target_detected:
        print("FPS:{:.1f} Target:({},{}) Aligned:{} Laser:{}".format(
            fps, cx, cy, aligned, laser_pulse_state))
