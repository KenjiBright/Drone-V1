# Drone V1 - Black Box Log (SPIFFS)

## Tổng quan

Black Box ghi dữ liệu bay vào **bộ nhớ flash nội bộ (SPIFFS)** ở định dạng CSV, tốc độ **50Hz**.
Ghi nền (background) trên Core 1 song song với flight loop, không cần kết nối Serial khi bay.

---

## Bật / Dừng Log

| Hành động | Cách làm |
|---|---|
| Bật log | Menu chính → `1` → nhập tên mô tả (Enter để dùng mặc định `log`) |
| Dừng log | Nhấn `E` |
| Xem danh sách file | Menu chính → `4` |
| Xuất CSV qua Serial | Menu chính → `4` → nhập tên file |
| Xóa file cũ | Menu chính → `5` → nhập tên file → xác nhận `YES` |

Khi bật: còi kêu 1 tiếng, về Menu chính (log tiếp tục chạy nền).
Khi dừng: hiển thị số byte đã ghi và tổng thời gian.

### Tên file

File được tự đặt tên theo công thức:
```
/bb_XXXXXX_XXXXXX_<mô tả>.csv
```
Vi dụ: `/bb_000042_000001_log.csv`

---

## Định dạng CSV

### Header
```
time_ms,thr,roll_sp,roll_gyro,roll_P,roll_I,roll_D,pitch_sp,pitch_gyro,pitch_P,pitch_I,pitch_D,yaw_sp,yaw_gyro,yaw_P,yaw_I,yaw_D
```

### Giải thích các cột

| Cột | Đơn vị | Mô tả |
|---|---|---|
| `time_ms` | ms | Thời gian kể từ khi khởi động |
| `thr` | µs (1000–2000) | Throttle hiện tại |
| `roll_sp` | °/s | **Setpoint** Roll — tốc độ góc mong muốn từ RC |
| `roll_gyro` | °/s | **Thực tế** Roll — dữ liệu gyro đọc về (đã lọc) |
| `roll_P` | — | Thành phần P của PID Roll |
| `roll_I` | — | Thành phần I của PID Roll |
| `roll_D` | — | Thành phần D của PID Roll (đã qua LPF) |
| `pitch_sp` | °/s | Setpoint Pitch |
| `pitch_gyro` | °/s | Thực tế Pitch |
| `pitch_P` | — | P-term Pitch |
| `pitch_I` | — | I-term Pitch |
| `pitch_D` | — | D-term Pitch |
| `yaw_sp` | °/s | Setpoint Yaw |
| `yaw_gyro` | °/s | Thực tế Yaw |
| `yaw_P` | — | P-term Yaw |
| `yaw_I` | — | I-term Yaw |
| `yaw_D` | — | D-term Yaw |

### Ví dụ một dòng dữ liệu
```
12345,1500,0.000,0.120,0.540,0.006,0.018,0.000,-0.050,-0.225,0.004,-0.007,0.000,0.031,0.078,0.002,0.000
```

---

## Đọc bằng Excel

### Cách 1: Xuất qua Serial

1. Kết nối Serial Monitor (115200)
2. Menu → `4` → nhập tên file (copy từ danh sách)
3. Kéo chọn toàn bộ dữ liệu giữa `---BEGIN CSV---` và `---END CSV---`
4. Copy (Ctrl+C), mở Excel → ô A1 → **Paste**
5. Chọn cột A → tab **Data** → **Text to Columns** → Delimited → Comma → Finish

### Cách 2: Download file CSV trực tiếp (khuyến nghị)

Sử dụng script `tools/download_spiffs_csv.py` để tải file từ ESP32 về máy tính. Xem [tools/README.md](../tools/README.md).

---

## Phân tích biểu đồ

### Kiểm tra đáp ứng PID

| Biểu đồ | Quan sát |
|---|---|
| `roll_sp` vs `roll_gyro` | Drone có theo kịp setpoint không? |
| Khoảng cách lớn | Kp quá thấp → phản ứng chậm |
| `roll_gyro` vượt qua `roll_sp` nhiều | Kp quá cao hoặc Kd quá thấp → overshoot |
| `roll_gyro` dao động quanh `roll_sp` | Kp quá cao → oscillation |

### Kiểm tra từng thành phần

| Biểu đồ | Dấu hiệu bất thường |
|---|---|
| `roll_P` | Dao động liên tục → Kp quá cao |
| `roll_I` | Tăng dần không hồi phục → Ki quá cao / windup |
| `roll_D` | Spike nhiễu liên tục → Kd quá cao hoặc cần tăng D-filter (`rf`) |

### Drone được tune tốt
- `roll_gyro` bám sát `roll_sp` với độ trễ nhỏ
- Không dao động
- `roll_D` mịn, không nhiễu spike
- `roll_I` ổn định, không tích lũy một chiều

---

## Lưu ý

- Log rate **50Hz** — đủ để phân tích PID ở flight loop 250Hz
- Dữ liệu chỉ lưu trong Serial Monitor, không lưu flash/SD
- Nên log ít nhất **30 giây bay hover** để có đủ mẫu phân tích
