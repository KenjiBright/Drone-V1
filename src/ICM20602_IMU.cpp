#include "ICM20602_IMU.h"
#include "Calibration.h"

// ===== REGISTER MAP ICM20602 =====
#define WHO_AM_I 0x75
#define PWR_MGMT_1 0x6B
#define ACCEL_XOUT_H 0x3B
#define GYRO_XOUT_H 0x43
#define GYRO_CONFIG 0x1B
#define ACCEL_CONFIG 0x1C
#define CONFIG 0x1A
#define ACCEL_CONFIG2 0x1D

// ===== INTERNAL STATE =====
static SPISettings spi_settings(ICM20602_SPI_SPEED, MSBFIRST, SPI_MODE0);

// Calib offsets
static float calib_rate_roll = 0;
static float calib_rate_pitch = 0;
static float calib_rate_yaw = 0;

// Sensor readings
static float rate_roll = 0, rate_pitch = 0, rate_yaw = 0;
static float acc_x = 0, acc_y = 0, acc_z = 0;

// Angle estimation (simple integration + accel)
static float angle_roll = 0, angle_pitch = 0;

// Kalman filter state
static float kalman_angle_roll = 0, kalman_unc_roll = 2*2;
static float kalman_angle_pitch = 0, kalman_unc_pitch = 2*2;

// Health check
static int imu_error_count = 0;
static bool imu_healthy = false;
static const int IMU_ERROR_THRESHOLD = 10;

// ===== LOW LEVEL SPI FUNCTIONS =====
static inline void writeRegister(uint8_t reg, uint8_t value) {
  digitalWrite(ICM20602_CS_PIN, LOW);
  SPI.beginTransaction(spi_settings);
  SPI.transfer(reg & 0x7F);  // Write: MSB = 0
  SPI.transfer(value);
  SPI.endTransaction();
  digitalWrite(ICM20602_CS_PIN, HIGH);
}

static inline uint8_t readRegister(uint8_t reg) {
  digitalWrite(ICM20602_CS_PIN, LOW);
  SPI.beginTransaction(spi_settings);
  SPI.transfer(reg | 0x80);  // Read: MSB = 1
  uint8_t value = SPI.transfer(0x00);
  SPI.endTransaction();
  digitalWrite(ICM20602_CS_PIN, HIGH);
  return value;
}

static inline void readAccel(int16_t &x, int16_t &y, int16_t &z) {
  digitalWrite(ICM20602_CS_PIN, LOW);
  SPI.beginTransaction(spi_settings);
  SPI.transfer(ACCEL_XOUT_H | 0x80);
  x = (int16_t)((SPI.transfer(0x00) << 8) | SPI.transfer(0x00));
  y = (int16_t)((SPI.transfer(0x00) << 8) | SPI.transfer(0x00));
  z = (int16_t)((SPI.transfer(0x00) << 8) | SPI.transfer(0x00));
  SPI.endTransaction();
  digitalWrite(ICM20602_CS_PIN, HIGH);
}

static inline void readGyro(int16_t &x, int16_t &y, int16_t &z) {
  digitalWrite(ICM20602_CS_PIN, LOW);
  SPI.beginTransaction(spi_settings);
  SPI.transfer(GYRO_XOUT_H | 0x80);
  x = (int16_t)((SPI.transfer(0x00) << 8) | SPI.transfer(0x00));
  y = (int16_t)((SPI.transfer(0x00) << 8) | SPI.transfer(0x00));
  z = (int16_t)((SPI.transfer(0x00) << 8) | SPI.transfer(0x00));
  SPI.endTransaction();
  digitalWrite(ICM20602_CS_PIN, HIGH);
}

// ===== KALMAN FILTER (1D) =====
static inline void kalman_update(float &state, float &uncertainty, float rate_input, float accel_measurement, float dt) {
  state = state + rate_input * dt;
  uncertainty = uncertainty + dt * dt * 1.0f * 1.0f;
  
  float gain = uncertainty * 1.0f / (1.0f * uncertainty + 3.0f * 3.0f);
  state = state + gain * (accel_measurement - state);
  uncertainty = (1.0f - gain) * uncertainty;
}

// ===== SENSOR READ & PROCESS =====
static inline void readSensorData(float dt) {
  int16_t ax, ay, az;
  readAccel(ax, ay, az);
  
  int16_t gx, gy, gz;
  readGyro(gx, gy, gz);
  
  // Convert to physical units
  // Accel: ±8g -> divide by 4096
  acc_x = (float)ax / 4096.0f;
  acc_y = (float)ay / 4096.0f;
  acc_z = (float)az / 4096.0f;
  
  // Gyro: ±500dps -> divide by 65.5
  rate_roll = (float)gx / 65.5f;
  rate_pitch = (float)gy / 65.5f;
  rate_yaw = (float)gz / 65.5f;
  
  // Sanity check
  if (fabsf(rate_roll) > 500 || fabsf(rate_pitch) > 500 || fabsf(rate_yaw) > 500 ||
      fabsf(acc_x) > 16 || fabsf(acc_y) > 16 || fabsf(acc_z) > 16) {
    imu_error_count++;
    return;
  }
  
  imu_error_count = 0;  // Reset on success
  
  // Calculate angle from accel
  float acc_angle_roll = atanf(acc_y / sqrtf(acc_x*acc_x + acc_z*acc_z)) * 57.2958f;
  float acc_angle_pitch = -atanf(acc_x / sqrtf(acc_y*acc_y + acc_z*acc_z)) * 57.2958f;
  
  // Kalman fusion
  kalman_update(kalman_angle_roll, kalman_unc_roll, rate_roll - calib_rate_roll, acc_angle_roll, dt);
  kalman_update(kalman_angle_pitch, kalman_unc_pitch, rate_pitch - calib_rate_pitch, acc_angle_pitch, dt);
}

// ===== INITIALIZATION =====
void IMU_begin() {
  // Configure SPI pins (these are ESP32 VSPI defaults)
  SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN, ICM20602_CS_PIN);
  SPI.setFrequency(ICM20602_SPI_SPEED);
  
  // Configure CS pin manually
  pinMode(ICM20602_CS_PIN, OUTPUT);
  digitalWrite(ICM20602_CS_PIN, HIGH);
  
  delay(100);
  
  // Reset and configure ICM20602
  writeRegister(PWR_MGMT_1, 0x00);  // Wake up
  delay(100);
  
  writeRegister(CONFIG, 0x06);        // DLPF: ~5.1 Hz
  writeRegister(ACCEL_CONFIG, 0x10);  // ±8g
  writeRegister(GYRO_CONFIG, 0x08);   // ±500dps
  writeRegister(ACCEL_CONFIG2, 0x06); // Accel DLPF
  delay(100);
  
  imu_healthy = ICM20602_check_connection();
  if (imu_healthy) {
    Serial.println("[IMU] ICM20602 connected!");
  } else {
    Serial.println("[IMU] WARNING: ICM20602 not detected!");
  }
}

// ===== CALIBRATION =====
void IMU_calibrate() {
  // Load from EEPROM if available
  CalibrationData_t* calib_data = Calibration_get_data();
  
  if (calib_data && calib_data->magic == 0xCAFE) {
    calib_rate_roll = calib_data->gyro_offset[0];
    calib_rate_pitch = calib_data->gyro_offset[1];
    calib_rate_yaw = calib_data->gyro_offset[2];
    Serial.printf("[IMU] Loaded stored gyro offsets\n");
  } else {
    // Quick calibration if no stored data
    Serial.print("[IMU] Quick Gyro Calibration...");
    for (int i = 0; i < 500; i++) {
      readSensorData(0.002f);
      calib_rate_roll += rate_roll;
      calib_rate_pitch += rate_pitch;
      calib_rate_yaw += rate_yaw;
      delay(1);
    }
    calib_rate_roll /= 500.0f;
    calib_rate_pitch /= 500.0f;
    calib_rate_yaw /= 500.0f;
    Serial.println(" Done!");
  }
  
  imu_error_count = 0;
}

// ===== UPDATE (Main sensor loop) =====
void IMU_update(float dt) {
  readSensorData(dt);
  
  // Update the public angle values for output
  angle_roll = kalman_angle_roll;
  angle_pitch = kalman_angle_pitch;
}

// ===== HEALTH CHECK =====
bool IMU_is_healthy() {
  return imu_healthy && imu_error_count < IMU_ERROR_THRESHOLD;
}

bool ICM20602_check_connection() {
  uint8_t who = readRegister(WHO_AM_I);
  if (who == 0x12) {
    return true;
  }
  return false;
}

// ===== GETTER FUNCTIONS =====
float IMU_get_rate_roll() { return rate_roll - calib_rate_roll; }
float IMU_get_rate_pitch() { return rate_pitch - calib_rate_pitch; }
float IMU_get_rate_yaw() { return rate_yaw - calib_rate_yaw; }

float IMU_get_rate_roll_raw()  { return rate_roll; }
float IMU_get_rate_pitch_raw() { return rate_pitch; }
float IMU_get_rate_yaw_raw()   { return rate_yaw; }

float IMU_get_angle_roll() { return angle_roll; }
float IMU_get_angle_pitch() { return angle_pitch; }
