# Drone V1 - Tools

Công cụ hỗ trợ cho Drone V1 Flight Controller.

## 📊 capture_blackbox.py

Python script để capture, lưu và phân tích Black Box data từ drone.

### Cài đặt

```bash
# Cài đặt dependencies
pip install -r requirements.txt
```

### Sử dụng

#### 1. Capture dữ liệu mới (30 giây)

```bash
python capture_blackbox.py --port COM17 --duration 30 --desc "test_pid"
```

**Khi chạy:**
1. Serial port sẽ kết nối
2. Drone sẽ hiển thị menu
3. Nhấn `1` để bắt đầu Black Box log
4. Script sẽ capture trong 30 giây
5. Nhấn `E` trên drone để dừng log
6. Biểu đồ sẽ tự động vẽ và hiển thị

**Tham số:**
- `--port` : Serial port (default: COM17)
- `--baudrate` : Baud rate (default: 115200)
- `--duration` : Thời gian capture (giây, default: 30)
- `--desc` : Mô tả flight (default: flight)
- `--no-plot` : Không vẽ biểu đồ (chỉ lưu CSV)

#### 2. Phân tích file CSV cũ

```bash
python capture_blackbox.py --analyze flight_20260412_143630_test_pid.csv
```

Sẽ đọc file CSV, tính thống kê, vẽ biểu đồ.

### Output

Script tạo ra:

1. **flight_YYYYMMDD_HHMMSS_description.csv** — Dữ liệu CSV
2. **flight_YYYYMMDD_HHMMSS_description.png** — Biểu đồ phân tích

### CSV Format

```
time_ms,thr,roll_sp,roll_gyro,roll_P,roll_I,roll_D,pitch_sp,pitch_gyro,pitch_P,pitch_I,pitch_D,yaw_sp,yaw_gyro,yaw_P,yaw_I,yaw_D
12345,1500,0.000,0.120,0.540,0.006,0.018,0.000,-0.050,-0.225,0.004,-0.007,0.000,0.031,0.078,0.002,0.000
```

### Biểu đồ Output

Vẽ 6 subplot:
- **Roll Response** — Setpoint vs Actual gyro
- **Roll PID Components** — P, I, D terms
- **Pitch Response** — Setpoint vs Actual gyro
- **Pitch PID Components** — P, I, D terms
- **Yaw Response** — Setpoint vs Actual gyro
- **Yaw PID Components** — P, I, D terms

### Thống kê

Tính toán:
- Thời gian bay (s)
- Throttle avg/max
- Error max, RMS cho từng trục (Roll/Pitch/Yaw)

### Troubleshooting

| Lỗi | Giải pháp |
|---|---|
| `ModuleNotFoundError: No module named 'serial'` | `pip install pyserial` |
| `ModuleNotFoundError: No module named 'pandas'` | `pip install pandas matplotlib` |
| `Serial port COM17 not found` | Thay `--port` thành COM port đúng (kiểm tra Device Manager) |
| `Timeout waiting for CSV header` | Kiểm tra drone gửi data đúng (menu → 1 → L) |

### Ví dụ đầu ra

```
============================================================
🚁 DRONE V1 - BLACK BOX CAPTURE TOOL
============================================================
✓ Kết nối COM17 @ 115200 baud

============================================================
📊 CAPTURE BLACK BOX DATA
============================================================
Thời gian: 30 giây
File: flight_20260412_143630_test_pid.csv

⏳ Hãy nhấn phím 1 trên menu drone để bắt đầu log...

✓ Tìm thấy CSV header
[████████████████████████████████████████] 100% (150 rows)

✓ Capture hoàn tất!
  Dòng: 150
  File: flight_20260412_143630_test_pid.csv

============================================================
📈 PHÂN TÍCH DỮ LIỆU
============================================================

✓ Đọc 150 dòng dữ liệu

📊 THỐNG KÊ:
  Thời gian bay: 30.0s
  Throttle avg: 1450 µs
  Throttle max: 1950 µs

🎛️ PID PERFORMANCE:
  Roll:
    Error max: 2.15°
    Error RMS: 0.68°
  Pitch:
    Error max: 1.95°
    Error RMS: 0.61°
  Yaw:
    Error max: 3.50°
    Error RMS: 1.23°

📊 Vẽ biểu đồ...
✓ Biểu đồ lưu: flight_20260412_143630_test_pid.png

============================================================
✓ Xong!
============================================================
```

---

## 📝 Ghi chú

- Capture chỉ hoạt động khi drone đang gửi dữ liệu qua Serial
- CSV header phải bắt đầu bằng `time_ms,thr,roll_sp,...`
- Tất cả giá trị số được lưu dưới dạng chuỗi text, phân tách bằng dấu phẩy
- Biểu đồ PNG được lưu cùng thư mục với CSV

---

**Version:** 1.0  
**Last Updated:** April 12, 2026
