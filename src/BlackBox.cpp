#include "BlackBox.h"

static bool    bb_active  = false;
static uint8_t bb_counter = 0;

void BlackBox_init() {
    bb_active  = false;
    bb_counter = 0;
}

void BlackBox_toggle() {
    bb_active = !bb_active;
    bb_counter = 0;

    if (bb_active) {
        Serial.println("\n[BLACKBOX] BẮT ĐẦU LOG - Gửi 'L' để dừng");
        Serial.println(
            "time_ms,thr,"
            "roll_sp,roll_gyro,roll_P,roll_I,roll_D,"
            "pitch_sp,pitch_gyro,pitch_P,pitch_I,pitch_D,"
            "yaw_sp,yaw_gyro,yaw_P,yaw_I,yaw_D"
        );
    } else {
        Serial.println("\n[BLACKBOX] DỪNG LOG");
    }
}

bool BlackBox_is_active() {
    return bb_active;
}

void BlackBox_log(
    int   throttle,
    float roll_sp,  float roll_gyro,  float roll_P,  float roll_I,  float roll_D,
    float pitch_sp, float pitch_gyro, float pitch_P, float pitch_I, float pitch_D,
    float yaw_sp,   float yaw_gyro,   float yaw_P,   float yaw_I,   float yaw_D
) {
    if (!bb_active) return;

    if (++bb_counter < BLACKBOX_LOG_DIVIDER) return;
    bb_counter = 0;

    Serial.printf(
        "%lu,%d,"
        "%.3f,%.3f,%.3f,%.3f,%.3f,"
        "%.3f,%.3f,%.3f,%.3f,%.3f,"
        "%.3f,%.3f,%.3f,%.3f,%.3f\n",
        millis(), throttle,
        roll_sp,  roll_gyro,  roll_P,  roll_I,  roll_D,
        pitch_sp, pitch_gyro, pitch_P, pitch_I, pitch_D,
        yaw_sp,   yaw_gyro,   yaw_P,   yaw_I,   yaw_D
    );
}
