# Drone V1 - Hướng dẫn cài đặt điều khiển TX

## Yêu cầu

- Điều khiển (TX) hỗ trợ **SBUS** (FrSky, RadioMaster, Jumper, v.v.)
- Receiver (RX) có đầu ra SBUS → nối vào **GPIO 35** trên ESP32
- Protocol: 100kbaud, 8E2, inverted (tự động xử lý trong firmware)

---

## Chế độ stick: Mode 2 (AETR)

Firmware sử dụng thứ tự kênh **AETR** (chuẩn quốc tế Mode 2):

```
         TX Mặt trước
   ┌─────────────────────────┐
   │                         │
   │   ┌───┐       ┌───┐    │
   │   │ ↑ │ THR   │ ↑ │ PIT│
   │   │   │       │   │    │
   │ ←─┤   ├─→   ←─┤   ├─→ │
   │ YAW│   │  ROLL │   │   │
   │   │ ↓ │       │ ↓ │    │
   │   └───┘       └───┘    │
   │  Stick trái  Stick phải│
   └─────────────────────────┘
```

| Stick | Hướng | SBUS Channel | Chức năng | Dải |
|---|---|---|---|---|
| Phải ← → | Ngang | CH1 | **Roll** (Aileron) | ±30° |
| Phải ↑ ↓ | Dọc | CH2 | **Pitch** (Elevator) | ±30° |
| Trái ↑ ↓ | Dọc | CH3 | **Throttle** | 1000–2000 µs |
| Trái ← → | Ngang | CH4 | **Yaw** (Rudder) | ±50°/s |

> **Lưu ý:** Nếu TX dùng thứ tự khác (TAER, RTAE...), cần đổi channel mapping trong TX cho đúng AETR.

---

## Switch và AUX channel

| SBUS Channel | Chức năng | Gán trên TX | Giá trị |
|---|---|---|---|
| **CH5** | **ARM + LED** | Switch 2 vị trí (ví dụ: SWA hoặc SWB) | >1500µs = ARM + LED ON / <1500µs = DISARM + LED OFF |

### Cài đặt ARM switch trên TX

1. Vào **MIXER** hoặc **MIXES** trên TX
2. Tìm **CH5**
3. Gán nguồn (Source) = **SA**, **SB**, hoặc switch 2 vị trí bất kỳ
4. Đặt Weight = **100%**
5. Đảm bảo:
   - Switch **DOWN** (vị trí gần người) = giá trị thấp (~1000µs) = **DISARM**
   - Switch **UP** (vị trí xa người) = giá trị cao (~2000µs) = **ARM**

### Quy trình Arm an toàn

```
1. Bật TX → bật RX/drone
2. Đặt throttle stick THẤP NHẤT (hoàn toàn xuống dưới)
3. Chờ 2 tiếng bíp (boot hoàn tất)
4. Gạt ARM switch lên (CH5 > 1500)
5. Từ từ đẩy throttle lên để drone cất cánh
```

### Quy trình Disarm

```
1. Hạ throttle xuống thấp nhất
2. Gạt ARM switch xuống (CH5 < 1500)
   → Motor dừng ngay lập tức
   → LED tắt
```

---

## Bảo vệ tự động (Failsafe)

Drone sẽ **tự động disarm** (tắt motor) trong các trường hợp:

| Tình huống | Phát hiện |
|---|---|
| Mất tín hiệu TX | Không nhận SBUS frame > 100ms |
| RX báo failsafe | Byte flag trong SBUS frame |
| Drone lật > 85° | IMU phát hiện góc Roll hoặc Pitch vượt ngưỡng |
| IMU lỗi | Dữ liệu sensor bất thường |

### Cài đặt Failsafe trên TX/RX

**Khuyến nghị:** Cài failsafe trên RX để **không gửi tín hiệu** (No Pulses) khi mất kết nối. Firmware sẽ phát hiện timeout và tắt motor.

Với FrSky:
1. TX → MODEL SETUP → FAILSAFE → **No Pulses**

Với RadioMaster (ELRS):
1. Failsafe mặc định là No Pulses — không cần cài thêm

---

## Deadzone và LPF

| Thông số | Giá trị | Mô tả |
|---|---|---|
| Deadzone Roll/Pitch | ±1.5° | Stick nhỏ hơn ngưỡng này → giá trị = 0 |
| Deadzone Yaw | ±5.0° | Yaw nhỏ hơn ngưỡng này → giá trị = 0 |
| LPF Alpha | 0.15 | Bộ lọc input (cao = nhạy hơn, thấp = mượt hơn) |

> Deadzone giúp drone giữ yên khi stick ở giữa. Nếu drone vẫn drift nhẹ khi thả stick → cần calibrate gyro.

---

## Dải tín hiệu

```
SBUS raw:      173 ─────── 992 ─────── 1811
Mapped (µs):   990 ─────── 1500 ────── 2010
Ý nghĩa:       MIN         CENTER       MAX
```

| Kênh | Min (990µs) | Center (1500µs) | Max (2010µs) |
|---|---|---|---|
| CH1 Roll | Nghiêng trái -30° | Giữ thẳng 0° | Nghiêng phải +30° |
| CH2 Pitch | Ngửa ra sau -30° | Giữ thẳng 0° | Chúi tới +30° |
| CH3 Throttle | Motor tắt 1000µs | Nửa ga | Ga max 2000µs |
| CH4 Yaw | Xoay trái -50°/s | Giữ yên 0 | Xoay phải +50°/s |
| CH5 ARM | DISARM (<1500) | — | ARM (>1500) |

---

## Trim và Subtrim

**KHÔNG sử dụng trim trên TX.** Firmware có PID I-term tự bù offset. Nếu dùng trim TX sẽ gây xung đột với PID.

Nếu drone nghiêng 1 bên khi hover:
1. Kiểm tra trọng tâm cơ khí
2. Calibrate gyro lại (Menu → 2 → 1)
3. Tăng Ki nhẹ nếu cần (Menu → 3 → `ri 0.10`)

---

## Kiểm tra kết nối

Sau khi gán channel trên TX, kiểm tra bằng cách:

1. Kết nối Serial Monitor (115200 baud)
2. Vào Menu → 1 (Black Box Log)
3. Quan sát cột `thr`: di chuyển throttle stick → giá trị thay đổi 1000–2000
4. Quan sát cột `roll_sp` / `pitch_sp`: di chuyển stick phải → giá trị thay đổi ±30
5. Gạt ARM switch → LED sáng

Nếu giá trị không phản ứng đúng → kiểm tra lại thứ tự channel trên TX.
