# Drone V1 - Calibration

## Truy cập

Từ Menu chính nhấn `2` → còi kêu 1 tiếng.

```
================== MENU CALIBRATION DRONE ==================
1) Calibration Gyro IMU
2) Calibration Accel IMU (6 điểm)
3) Calibration ESC Min/Max
4) Kiểm tra toàn dãy ESC tự động
5) Tải Calibration từ EEPROM
6) Xóa tất cả Calibration
7) Hiển thị Dữ liệu Calibration hiện tại
==========================================================
```

Nhấn `E` để thoát về Menu chính.

---

## 1. Calibration Gyro IMU (lệnh `1`)

### Khi nào cần
- Lần đầu sử dụng
- Drone bị drift (tự nghiêng) khi đứng yên
- Sau khi thay IMU hoặc thay đổi phần cứng

### Quy trình

1. Đặt drone trên mặt **bằng phẳng, tuyệt đối yên tĩnh**
2. Nhấn `1` + Enter → còi kêu 1 tiếng
3. **Không chạm, không rung, không gió** trong ~10 giây
4. Hệ thống thu thập 1000 mẫu gyro và tính offset
5. **Còi kêu 3 tiếng** → hoàn tất
6. Dữ liệu tự động lưu vào EEPROM

### Kết quả hiển thị
```
========== CALIBRATION GYRO HOAN THANH ==========
Gia tri Offset Gyro:
  Roll:  0.023 °/s
  Pitch: -0.011 °/s
  Yaw:   0.005 °/s
```

> **Lưu ý:** Offset nhỏ (< 1 °/s) là bình thường. Nếu > 2 °/s, kiểm tra lại vị trí đặt drone và làm lại.

---

## 2. Calibration ESC Min/Max (lệnh `3`)

### Khi nào cần
- ESC mới chưa được calibrate
- Motor không phản hồi đúng dải gas (1000–2000µs)
- Sau khi thay ESC

> ⚠️ **THÁO CÁNH QUẠT hoặc ngắt pin motor trước khi thực hiện.**

### Quy trình

1. Nhấn `3` → còi kêu dài (cảnh báo), đọc hướng dẫn trên màn hình
2. Hệ thống gửi **throttle MAX (2000µs)** đến tất cả ESC trong 2 giây
3. Hệ thống chuyển sang **throttle MIN (1000µs)** trong 2 giây
4. **Còi kêu 3 tiếng** → hoàn tất
5. ESC đã học dải 1000–2000µs

---

## 3. Kiểm tra dãy ESC (lệnh `4`)

Chạy thử từ 1000µs lên 2000µs theo bước 100µs, mỗi bước dừng 500ms.

> ⚠️ **THÁO CÁNH QUẠT trước khi chạy.** Motor sẽ quay.

Dùng để xác nhận ESC và motor hoạt động đúng trên toàn dải.

---

## 4. Tải từ EEPROM (lệnh `5`)

Tải lại dữ liệu calibration đã lưu từ EEPROM vào RAM.
Dùng khi muốn khôi phục sau khi reset bộ nhớ tạm thời.

---

## 5. Xóa toàn bộ Calibration (lệnh `6`)

1. Nhấn `6`
2. Nhập `RESET` + Enter để xác nhận
3. **Còi kêu 3 tiếng** → xóa xong

> ⚠️ Thao tác không thể hoàn tác. Cần calibrate lại từ đầu.

---

## 6. Hiển thị dữ liệu (lệnh `7`)

In ra màn hình toàn bộ offset giá trị hiện đang áp dụng:
- Gyro offset (Roll/Pitch/Yaw)
- Accel offset và scale
- Thời gian calibration

---

## Lưu ý chung

- Dữ liệu calibration lưu trong **EEPROM**, không mất khi tắt nguồn
- Nên calibrate Gyro **mỗi khi thay đổi phần cứng hoặc môi trường nhiệt độ thay đổi lớn**
- Calibration ESC chỉ cần làm **một lần** với ESC mới
