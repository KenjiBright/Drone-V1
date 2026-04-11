#ifndef BLACKBOX_H
#define BLACKBOX_H

#include <Arduino.h>

// ===== BLACK BOX - SERIAL LOGGER =====
// Ghi log dữ liệu bay ra Serial Monitor định dạng CSV
// Log rate: 50Hz (mỗi 5 cycle ở 250Hz flight loop)
// Bật/Tắt: Gửi 'L' qua Serial
//
// Định dạng CSV:
// time_ms,thr,
// roll_sp,roll_gyro,roll_P,roll_I,roll_D,
// pitch_sp,pitch_gyro,pitch_P,pitch_I,pitch_D,
// yaw_sp,yaw_gyro,yaw_P,yaw_I,yaw_D

#define BLACKBOX_LOG_DIVIDER 5   // Log mỗi N cycle (250/5 = 50Hz)

void BlackBox_init();
void BlackBox_toggle();
bool BlackBox_is_active();

// Gọi trong flight loop mỗi cycle, tự lọc theo BLACKBOX_LOG_DIVIDER
void BlackBox_log(
    int   throttle,
    // Roll
    float roll_sp,  float roll_gyro,  float roll_P,  float roll_I,  float roll_D,
    // Pitch
    float pitch_sp, float pitch_gyro, float pitch_P, float pitch_I, float pitch_D,
    // Yaw
    float yaw_sp,   float yaw_gyro,   float yaw_P,   float yaw_I,   float yaw_D
);

#endif // BLACKBOX_H
