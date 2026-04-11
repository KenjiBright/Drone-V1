#include "Calibration.h"
#include "ICM20602_IMU.h"
#include "PWM_Out.h"
#include "Buzzer.h"
#include <EEPROM.h>
#include <Wire.h>

// ===== BỐ CỤC EEPROM =====
#define EEPROM_SIZE 512
#define CALIB_DATA_ADDR 0
#define CALIB_MAGIC 0xCAFE

// ===== BIẾN TĨNH =====
static CalibrationMode current_mode = CALIB_NONE;
static CalibrationData_t calib_data = {0};

// Biến calibration IMU
static float gyro_accum[3] = {0, 0, 0};
static float accel_accum[3] = {0, 0, 0};
static uint16_t calib_sample_count = 0;
static const uint16_t CALIB_SAMPLES = 2000;
static bool calib_in_progress = false;
static uint32_t calib_start_time = 0;

// Biến calibration ESC
static int esc_calib_step = 0;
static uint32_t esc_calib_timer = 0;

// ===== KHỞI TẠO =====
void Calibration_init() {
  EEPROM.begin(EEPROM_SIZE);
  
  // Cố gắng tải calibration đã lưu
  if (!Calibration_load_from_eeprom()) {
    Serial.println("WARNING: Không tìm thấy dữ liệu calibration hợp lệ. Dùng giá trị mặc định.");
    memset(&calib_data, 0, sizeof(CalibrationData_t));
    calib_data.magic = CALIB_MAGIC;
    calib_data.accel_scale[0] = calib_data.accel_scale[1] = calib_data.accel_scale[2] = 1.0f;
    Calibration_save_to_eeprom();
  } else {
    Serial.println("[OK] Dữ liệu calibration đã được tải từ EEPROM");
  }
}

// ===== QUẢN LÝ EEPROM =====
bool Calibration_load_from_eeprom() {
  EEPROM.get(CALIB_DATA_ADDR, calib_data);
  if (calib_data.magic != CALIB_MAGIC) {
    return false;
  }
  Serial.printf("Tải calibration từ %lu (%.2f ngày trước)\n", 
                calib_data.timestamp, 
                (millis() - calib_data.timestamp * 1000.0f) / 86400000.0f);
  return true;
}

bool Calibration_save_to_eeprom() {
  calib_data.timestamp = millis() / 1000;  // Lưu theo giây
  EEPROM.put(CALIB_DATA_ADDR, calib_data);
  EEPROM.commit();
  Serial.println("[OK] Dữ liệu calibration đã được lưu vào EEPROM");
  return true;
}

// ===== QUẢN LÝ CHỈ TIÊU =====
CalibrationMode Calibration_get_mode() {
  return current_mode;
}

void Calibration_set_mode(CalibrationMode mode) {
  current_mode = mode;
  calib_in_progress = false;
  calib_sample_count = 0;
}

// ===== HỆ THỐNG MENU =====
void Calibration_display_menu() {
  Serial.println("\n================== MENU CALIBRATION DRONE ==================");
  Serial.println("1) Calibration Gyro IMU (Giữ drone YÊN TĨNH trên mặt phẳng)");
  Serial.println("2) Calibration Accel IMU (6 điểm)");
  Serial.println("3) Calibration ESC Min/Max");
  Serial.println("4) Kiểm tra toàn dãy ESC tự động");
  Serial.println("5) Tải Calibration từ EEPROM");
  Serial.println("6) Xóa tất cả Calibration");
  Serial.println("7) Hiển thị Dữ liệu Calibration hiện tại");
  Serial.println("0) Thoát Menu Calibration");
  Serial.println("==========================================================");
  Serial.print("Chọn tùy chọn (0-7): ");
}

// ===== CALIBRATION GYRO IMU =====
void Calibration_imu_gyro_start() {
  current_mode = CALIB_IMU_GYRO;
  calib_in_progress = true;
  calib_sample_count = 0;
  gyro_accum[0] = gyro_accum[1] = gyro_accum[2] = 0;
  calib_start_time = millis();
  
  Serial.println("\n========== BẮTĐẦU CALIBRATION GYRO IMU ==========");
  Serial.println("Giữ drone YÊN TĨNH và BẰNG PHẲNG trên bề mặt phẳng");
  Serial.printf("Đang thu thập %d mẫu (chờ ~%.1f giây)...\n", CALIB_SAMPLES, CALIB_SAMPLES / 1000.0f);
}

void Calibration_imu_gyro_update() {
  if (current_mode != CALIB_IMU_GYRO || !calib_in_progress) return;
  
  // Đọc dữ liệu từ IMU (thường được gọi từ hàm update IMU chính)
  Wire.beginTransmission(0x68);
  Wire.write(0x43);  // Đọc gyro
  if (Wire.endTransmission(false) != 0) return;
  
  if (Wire.requestFrom(0x68, 6, true) != 6) return;
  
  gyro_accum[0] += (float)(Wire.read() << 8 | Wire.read()) / 65.5f;
  gyro_accum[1] += (float)(Wire.read() << 8 | Wire.read()) / 65.5f;
  gyro_accum[2] += (float)(Wire.read() << 8 | Wire.read()) / 65.5f;
  
  calib_sample_count++;
  
  if (calib_sample_count % 500 == 0) {
    Serial.printf("  Tiến độ: %d/%d mẫu\r", calib_sample_count, CALIB_SAMPLES);
  }
  
  if (calib_sample_count >= CALIB_SAMPLES) {
    Calibration_imu_gyro_complete();
  }
}

bool Calibration_imu_gyro_complete() {
  if (calib_sample_count == 0) return false;
  
  calib_data.gyro_offset[0] = gyro_accum[0] / calib_sample_count;
  calib_data.gyro_offset[1] = gyro_accum[1] / calib_sample_count;
  calib_data.gyro_offset[2] = gyro_accum[2] / calib_sample_count;
  
  Serial.println("\n========== CALIBRATION GYRO HOÀN THÀNH ==========");
  Serial.printf("Giá trị Offset Gyro:\n");
  Serial.printf("  Roll:  %.3f °/s\n", calib_data.gyro_offset[0]);
  Serial.printf("  Pitch: %.3f °/s\n", calib_data.gyro_offset[1]);
  Serial.printf("  Yaw:   %.3f °/s\n", calib_data.gyro_offset[2]);
  
  calib_in_progress = false;
  current_mode = CALIB_NONE;
  Calibration_save_to_eeprom();
  beep(100); delay(80); beep(100); delay(80); beep(100);  // 3 bip = calib xong
  
  return true;
}

// ===== CALIBRATION ACCEL IMU (6-ĐIỂM) =====
void Calibration_imu_accel_start() {
  current_mode = CALIB_IMU_ACCEL;
  calib_in_progress = true;
  calib_sample_count = 0;
  
  Serial.println("\n========== BẮTĐẦU CALIBRATION ACCEL 6-ĐIỂM ==========");
  Serial.println("Đặt drone ở các vị trí sau và nhấn ENTER:");
  Serial.println("1) Bằng phẳng (flat) - Z hướng lên");
  Serial.println("2) Z hướng xuống");
  Serial.println("3) X hướng phía trước");
  Serial.println("4) X hướng phía sau");
  Serial.println("5) Y hướng bên phải");
  Serial.println("6) Y hướng bên trái");
  Serial.println("Bắtđầu sau 5 giây...");
  delay(5000);
}

void Calibration_imu_accel_update() {
  if (current_mode != CALIB_IMU_ACCEL || !calib_in_progress) return;
  
  // Placeholder - calibration 6-điểm thực tế cần tương tác người dùng
  // Sẽ được kích hoạt thông qua menu serial
}

bool Calibration_imu_accel_complete() {
  // Hiện tại: lấy trung bình đơn giản - 6-điểm thực tế sẽ dùng toán ma trận
  if (calib_sample_count == 0) return false;
  
  Serial.println("[OK] Calibration accel hoàn thành");
  calib_in_progress = false;
  current_mode = CALIB_NONE;
  Calibration_save_to_eeprom();
  
  return true;
}

// ===== CALIBRATION ESC =====
void Calibration_esc_start() {
  current_mode = CALIB_ESC_MIN_MAX;
  calib_in_progress = true;
  esc_calib_step = 0;
  esc_calib_timer = millis();
  
  Serial.println("\n========== BẮTĐẦU CALIBRATION ESC ==========");
  Serial.println("Bước 1: Kết nối ESC với tay điều khiển throttle ở mức maximum");
  Serial.println("Bước 2: Cấp nguồn ESC (sẽ phát ra tiếng bíp tăng dần)");
  Serial.println("Bước 3: Hạ throttle xuống mức minimum (sẽ phát 2 tiếng bíp)");
  Serial.println("Chờ 3 giây trước khi gửi tín hiệu calibration...");
  delay(3000);
  
  // Gửi throttle cực đại đến tất cả ESC
  PWM_motor(2000, 2000, 2000, 2000, true);
}

void Calibration_esc_update() {
  if (current_mode != CALIB_ESC_MIN_MAX || !calib_in_progress) return;
  
  uint32_t elapsed = millis() - esc_calib_timer;
  
  if (elapsed < 2000) {
    // Bước 1: Gửi throttle cực đại (2000µs)
    PWM_motor(2000, 2000, 2000, 2000, true);
    if (elapsed % 500 == 0) {
      Serial.printf("Gửi throttle MAX: 2000µs (%lu ms)\r", elapsed);
    }
  } else if (elapsed < 4000) {
    // Bước 2: Chuyển đổi sang throttle cực tiểu
    PWM_motor(1000, 1000, 1000, 1000, true);
    if (elapsed % 500 == 0) {
      Serial.printf("Gửi throttle MIN: 1000µs (%lu ms)\r", elapsed);
    }
  } else {
    // Calibration complete
    Calibration_esc_complete();
  }
}

bool Calibration_esc_complete() {
  Serial.println("\n[OK] Calibration ESC Hoan Thanh!");
  Serial.println("ESC da duoc calibration voi dai 1000-2000us");
  calib_in_progress = false;
  current_mode = CALIB_NONE;
  beep(100); delay(80); beep(100); delay(80); beep(100);  // 3 bip = calib xong

  // Gui throttle trung binh
  PWM_motor(1500, 1500, 1500, 1500, true);
  delay(1000);
  PWM_motor(1000, 1000, 1000, 1000, false);

  return true;
}

// ===== LẤY DỮ LIỆU =====
float* Calibration_get_gyro_offset() {
  return calib_data.gyro_offset;
}

float* Calibration_get_accel_offset() {
  return calib_data.accel_offset;
}

CalibrationData_t* Calibration_get_data() {
  return &calib_data;
}

void Calibration_reset_all() {
  Serial.println("Đang xóa tất cả dữ liệu calibration...");
  memset(&calib_data, 0, sizeof(CalibrationData_t));
  calib_data.magic = CALIB_MAGIC;
  calib_data.accel_scale[0] = calib_data.accel_scale[1] = calib_data.accel_scale[2] = 1.0f;
  Calibration_save_to_eeprom();
  Serial.println("[OK] Tất cả dữ liệu calibration đã được xoá về giá trị mặc định");
}

void Calibration_display_data() {
  Serial.println("\n========== DỮ LIỆU CALIBRATION ==========");
  Serial.printf("Hợp lệ: %s\n", calib_data.magic == CALIB_MAGIC ? "CÓ" : "KHÔNG");
  Serial.printf("Tuổi: %lu giây\n\n", millis() / 1000 - calib_data.timestamp);
  
  Serial.println("Giá trị Offset Gyro (°/s):");
  Serial.printf("  Roll:  %8.3f\n", calib_data.gyro_offset[0]);
  Serial.printf("  Pitch: %8.3f\n", calib_data.gyro_offset[1]);
  Serial.printf("  Yaw:   %8.3f\n\n", calib_data.gyro_offset[2]);
  
  Serial.println("Giá trị Offset Accel (g):");
  Serial.printf("  X: %8.3f\n", calib_data.accel_offset[0]);
  Serial.printf("  Y: %8.3f\n", calib_data.accel_offset[1]);
  Serial.printf("  Z: %8.3f\n\n", calib_data.accel_offset[2]);
  
  Serial.println("Hệ số Scale Accel:");
  Serial.printf("  X: %8.3f\n", calib_data.accel_scale[0]);
  Serial.printf("  Y: %8.3f\n", calib_data.accel_scale[1]);
  Serial.printf("  Z: %8.3f\n", calib_data.accel_scale[2]);
  Serial.println("======================================\n");
}
