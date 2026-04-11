# Drone V1 - Tổng Quan

## Phần cứng

| Thành phần | Chi tiết |
|---|---|
| Vi điều khiển | ESP32 |
| IMU | ICM20602 (SPI) |
| Receiver | SBUS (Serial2, RX=GPIO16) |
| Motor FL | GPIO 27 (CCW) |
| Motor FR | GPIO 26 (CW) |
| Motor RR | GPIO 25 (CW) |
| Motor RL | GPIO 33 (CCW) |
| LED | GPIO 32 |
| Còi (Buzzer) | GPIO 14 |
| Baud rate Serial | 115200 |

---

## Khởi động

Khi cắm nguồn:
1. ESP32 khởi tạo tất cả module
2. **Còi kêu 2 tiếng** → sẵn sàng
3. Menu chính hiện ra trên Serial Monitor

```
==============================
         MENU CHINH
==============================
 1) Black Box Log (CSV)
 2) Calibration
 3) PID Tuning (Live)
  >> Nhan E de thoat bat ky mode nao
==============================
```

> **Lưu ý:** Flight control loop (250Hz) luôn chạy song song trên Core 1.
> Tất cả menu chỉ là giao diện cấu hình, không ảnh hưởng đến điều khiển bay.

---

## Thông báo Còi

| Tín hiệu | Ý nghĩa |
|---|---|
| 2 bíp ngắn khi boot | Khởi động thành công |
| 1 bíp ngắn | Lệnh thực thi thành công |
| 1 bíp dài (200ms) | Cảnh báo (trước cali ESC) |
| 3 bíp ngắn | Calibration hoàn tất |

---

## Tài liệu liên quan

- [BLACKBOX.md](BLACKBOX.md) — Hướng dẫn ghi log và đọc dữ liệu CSV
- [CALIBRATION.md](CALIBRATION.md) — Hướng dẫn calibration Gyro, ESC
- [PID_TUNING.md](PID_TUNING.md) — Hướng dẫn tune PID Live
