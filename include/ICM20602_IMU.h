#ifndef ICM20602_IMU_H
#define ICM20602_IMU_H
#include <Arduino.h>
#include <SPI.h>

// ===== PIN CONFIGURATION =====
#define ICM20602_CS_PIN 5       // Chip Select
#define SPI_SCK_PIN 18          // Clock
#define SPI_MISO_PIN 19         // Master In Slave Out
#define SPI_MOSI_PIN 23         // Master Out Slave In
#define ICM20602_SPI_SPEED 4000000  // 4 MHz

// ===== FUNCTION DECLARATIONS =====

// Initialize IMU
void IMU_begin();

// Load calibration data (from EEPROM or do quick calib)
void IMU_calibrate();

// Update IMU readings (must be called regularly)
void IMU_update(float dt);

// Health check
bool IMU_is_healthy();

// Getter functions - same as MPU6050 for compatibility
float IMU_get_rate_roll();
float IMU_get_rate_pitch();
float IMU_get_rate_yaw();
float IMU_get_angle_roll();
float IMU_get_angle_pitch();

// Hardware test
bool ICM20602_check_connection();

#endif
