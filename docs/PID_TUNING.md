# Drone V1 - PID Tuning (Live)

## Truy cập

Từ Menu chính nhấn `3` → còi kêu 1 tiếng.

Giá trị PID hiện tại hiển thị ngay:
```
  Roll:  Kp=4.500 Ki=0.0800 Kd=0.1500 | windup=200 out=400 Tf=0.0050
  Pitch: Kp=4.500 Ki=0.0800 Kd=0.1500 | windup=200 out=400 Tf=0.0050
  Yaw:   Kp=2.500 Ki=0.0500 Kd=0.0000 | windup=200 out=400 Tf=0.0050
```

Nhấn `E` để thoát về Menu chính.

> **Lưu ý:** Thay đổi PID có hiệu lực **ngay lập tức** — drone vẫn bay trong khi tune.

---

## Cú pháp lệnh

```
<truc><tham_so> <gia_tri>
```

Nhập lệnh + Enter. Còi kêu 1 tiếng xác nhận.

### Ký hiệu trục

| Trục | Ký hiệu |
|---|---|
| Roll | `r` |
| Pitch | `p` |
| Yaw | `y` |

### Ký hiệu tham số

| Tham số | Ký hiệu | Mô tả |
|---|---|---|
| Kp | `p` | Hệ số tỉ lệ (Proportional) |
| Ki | `i` | Hệ số tích phân (Integral) |
| Kd | `d` | Hệ số vi phân (Derivative) |
| Windup limit | `w` | Giới hạn tích lũy I-term |
| Output limit | `o` | Giới hạn tổng output PID |
| D filter Tf | `f` | Hằng số thời gian LPF cho D-term (giây) |

### Lệnh đặc biệt

| Lệnh | Mô tả |
|---|---|
| `cp` | Copy toàn bộ thông số Roll → Pitch |

### Ví dụ

```
rp 4.50      → Roll Kp = 4.50
ri 0.08      → Roll Ki = 0.08
rd 0.15      → Roll Kd = 0.15
pp 5.00      → Pitch Kp = 5.00
yi 0.03      → Yaw Ki = 0.03
rw 200       → Roll windup limit = 200
ro 400       → Roll output limit = 400
rf 0.010     → Roll D-filter Tf = 0.010s (lọc mạnh hơn)
cp           → Copy Roll → Pitch (tất cả 6 tham số)
```

---

## Giải thích tham số nâng cao

### Windup limit (`w`)
Giới hạn giá trị tích lũy của I-term.
- **Quá nhỏ:** I-term không đủ để bù offset → drone vẫn drift
- **Quá lớn:** I-term tích lũy lớn → drone phản ứng chậm, rồi bù quá mức
- Giá trị mặc định: **200**

### Output limit (`o`)
Clamp tổng output PID trước khi đưa vào motor mixer.
- Ngăn motor nhận giá trị quá lớn khi PID mất ổn định
- Giá trị mặc định: **400** (tương đương ±400µs so với throttle)

### D-filter Tf (`f`)
Hằng số thời gian của bộ lọc Low-Pass Filter cho D-term.
- D-term tính trên gyro → rất nhạy với nhiễu → cần lọc
- `Tf = 0.005s` → alpha ≈ 0.55 tại 250Hz (lọc vừa)
- **Tăng Tf** → lọc mạnh hơn, giảm nhiễu, nhưng D phản ứng chậm hơn
- **Giảm Tf** → lọc yếu hơn, D nhạy hơn, dễ nhiễu motor

| Tf | Đặc tính |
|---|---|
| 0.002 | Ít lọc, D nhạy |
| 0.005 | Cân bằng (mặc định) |
| 0.010 | Lọc nhiều, phù hợp motor rung |
| 0.020 | Lọc mạnh, D gần như tắt |

---

## Quy trình Tune PID

### Bước 1: Tune Kp

Bay hover ở độ cao an toàn, tăng dần Kp từ thấp lên.

| Triệu chứng | Nguyên nhân | Giải pháp |
|---|---|---|
| Phản ứng chậm, lơ mơ | Kp quá thấp | Tăng Kp |
| Rung tần số cao | Kp quá cao | Giảm Kp |
| Phản ứng nhanh, ổn định | ✓ Kp đúng | Giữ nguyên |

### Bước 2: Tune Kd

Khi Kp ổn định, thêm Kd để giảm overshoot.

| Triệu chứng | Nguyên nhân | Giải pháp |
|---|---|---|
| Vẫn overshoot sau khi đổi stick | Kd quá thấp | Tăng Kd |
| Motor rung, tiếng whine | Kd quá cao | Giảm Kd hoặc tăng `rf` |
| Motor rung nặng dù Kd thấp | Nhiễu D-term | Tăng `rf` (ví dụ `rf 0.010`) |

### Bước 3: Tune Ki

Thêm Ki nhỏ để bù offset khi hover.

| Triệu chứng | Nguyên nhân | Giải pháp |
|---|---|---|
| Drone tự nghiêng cố định khi hover | Ki quá thấp hoặc = 0 | Tăng Ki nhỏ |
| Drone trôi có gia tốc | Ki quá cao | Giảm Ki |

### Bước 4: Tune Yaw

Yaw điều khiển tốc độ xoay thay vì góc. Thường cần ít tune hơn.

| Triệu chứng | Giải pháp |
|---|---|
| Yaw phản ứng chậm | Tăng `yp` |
| Yaw rung khi thả stick | Giảm `yp` |
| Yaw tự xoay nhẹ | Thêm `yi` nhỏ (~0.02) |

---

## Giá trị mặc định và gợi ý bắt đầu

| Trục | Kp | Ki | Kd | Windup | Output | D-filter Tf |
|---|---|---|---|---|---|---|
| Roll | 4.5 | 0.08 | 0.15 | 200 | 400 | 0.005 |
| Pitch | 4.5 | 0.08 | 0.15 | 200 | 400 | 0.005 |
| Yaw | 2.5 | 0.05 | 0.0 | 200 | 400 | 0.005 |

> **Tip:** Tune Roll đến khi ổn → gõ `cp` để copy sang Pitch → tinh chỉnh Pitch nếu cần (khung không đối xứng hoặc trọng tâm lệch trước/sau).

---

## Kết hợp với Black Box

Tune hiệu quả nhất khi kết hợp với log:

1. Nhấn `E` → về Menu chính
2. Nhấn `1` → bật Black Box
3. Bay 30 giây
4. Nhấn `E` → dừng log
5. Copy CSV vào Excel, vẽ biểu đồ `roll_sp` vs `roll_gyro`
6. Phân tích → vào Menu PID Tuning → điều chỉnh
7. Lặp lại

Xem chi tiết phân tích CSV tại [BLACKBOX.md](BLACKBOX.md).
