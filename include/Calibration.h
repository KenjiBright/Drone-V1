#ifndef CALIBRATION_H
#define CALIBRATION_H
#include <Arduino.h>

// ===== CẤU TRÚC DỮ LIỆU CALIBRATION =====
typedef struct {
  uint16_t magic;                  // Số magic để xác thực dữ liệu EEPROM
  float gyro_offset[3];            // Giá trị offset của gyro (roll, pitch, yaw)
  float accel_offset[3];           // Giá trị offset của accel (x, y, z)  
  float accel_scale[3];            // Hệ số scale của accel
  uint32_t timestamp;              // Thời gian calibration
} CalibrationData_t;

// ===== CHỈ TIÊU CALIBRATION =====
enum CalibrationMode {
  CALIB_NONE = 0,
  CALIB_IMU_GYRO = 1,              // Calibration offset gyro (đứng yên)
  CALIB_IMU_ACCEL = 2,             // Calibration accelerometer (6 điểm)
  CALIB_ESC_MIN_MAX = 3,           // Calibration ESC min/max throttle
  CALIB_ESC_RANGE = 4              // Calibration toàn bộ dãy ESC
};

// ===== KHAI BÁO HÀM =====
void Calibration_init();
bool Calibration_load_from_eeprom();
bool Calibration_save_to_eeprom();
void Calibration_display_menu();
CalibrationMode Calibration_get_mode();
void Calibration_set_mode(CalibrationMode mode);

// Hàm calibration IMU
void Calibration_imu_gyro_start();
void Calibration_imu_gyro_update();
bool Calibration_imu_gyro_complete();
float* Calibration_get_gyro_offset();

void Calibration_imu_accel_start();
void Calibration_imu_accel_update();
bool Calibration_imu_accel_complete();
float* Calibration_get_accel_offset();

// Hàm calibration ESC
void Calibration_esc_start();
void Calibration_esc_update();
bool Calibration_esc_complete();

// Quản lý dữ liệu
CalibrationData_t* Calibration_get_data();
void Calibration_reset_all();
void Calibration_display_data();

#endif
