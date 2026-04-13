#include "SBUS_Receiver.h"

// ===== CẤU HÌNH =====
#define DEADZONE_ANGLE 1.5f     // Deadzone cho stick inputs (độ)
#define DEADZONE_YAW 5.0f       // Deadzone cho điều khiển yaw
#define FILTER_ALPHA 0.15f      // Low-pass filter alpha (cao hơn = responsive hơn)

static HardwareSerial* sbus_port = nullptr;
static uint8_t sbus_buf[25];
static uint8_t sbus_idx = 0;
static uint16_t sbus_ch_raw[16] = {0};
static uint32_t sbus_last_frame_ms = 0;
static int parse_errors = 0;
static bool sbus_frame_lost = false;
static bool sbus_failsafe_flag = false;

void SBUS_begin(HardwareSerial &port, int rxPin) {
  sbus_port = &port;
  sbus_idx = 0;
  sbus_port->begin(100000, SERIAL_8E2, rxPin, -1, true); // true = Hardware Inversion
  while (sbus_port->available()) sbus_port->read();
  sbus_last_frame_ms = millis();
}

bool SBUS_update() {
  if (!sbus_port) return false;
  while (sbus_port->available()) {
    uint8_t c = (uint8_t)sbus_port->read();
    if (sbus_idx == 0 && c != 0x0F) continue; 
    sbus_buf[sbus_idx++] = c;

    if (sbus_idx >= 25) {
      sbus_idx = 0;
      // Xác thực frame (chỉ kiểm tra start byte, end byte có thể khác 0x00 tùy receiver)
      if (sbus_buf[0] != 0x0F) {
        parse_errors++;
        return false;
      }
      
      // Parse tat ca 16 channels
      sbus_ch_raw[0]  = ((sbus_buf[1]       | (sbus_buf[2]  << 8))                          & 0x07FF);
      sbus_ch_raw[1]  = ((sbus_buf[2]  >> 3  | (sbus_buf[3]  << 5))                          & 0x07FF);
      sbus_ch_raw[2]  = ((sbus_buf[3]  >> 6  | (sbus_buf[4]  << 2) | (sbus_buf[5]  << 10))   & 0x07FF);
      sbus_ch_raw[3]  = ((sbus_buf[5]  >> 1  | (sbus_buf[6]  << 7))                          & 0x07FF);
      sbus_ch_raw[4]  = ((sbus_buf[6]  >> 4  | (sbus_buf[7]  << 4))                          & 0x07FF);
      sbus_ch_raw[5]  = ((sbus_buf[7]  >> 7  | (sbus_buf[8]  << 1) | (sbus_buf[9]  << 9))    & 0x07FF);
      sbus_ch_raw[6]  = ((sbus_buf[9]  >> 2  | (sbus_buf[10] << 6))                          & 0x07FF);
      sbus_ch_raw[7]  = ((sbus_buf[10] >> 5  | (sbus_buf[11] << 3))                          & 0x07FF);
      sbus_ch_raw[8]  = ((sbus_buf[12]       | (sbus_buf[13] << 8))                          & 0x07FF);
      sbus_ch_raw[9]  = ((sbus_buf[13] >> 3  | (sbus_buf[14] << 5))                          & 0x07FF);
      sbus_ch_raw[10] = ((sbus_buf[14] >> 6  | (sbus_buf[15] << 2) | (sbus_buf[16] << 10))   & 0x07FF);
      sbus_ch_raw[11] = ((sbus_buf[16] >> 1  | (sbus_buf[17] << 7))                          & 0x07FF);
      sbus_ch_raw[12] = ((sbus_buf[17] >> 4  | (sbus_buf[18] << 4))                          & 0x07FF);
      sbus_ch_raw[13] = ((sbus_buf[18] >> 7  | (sbus_buf[19] << 1) | (sbus_buf[20] << 9))    & 0x07FF);
      sbus_ch_raw[14] = ((sbus_buf[20] >> 2  | (sbus_buf[21] << 6))                          & 0x07FF);
      sbus_ch_raw[15] = ((sbus_buf[21] >> 5  | (sbus_buf[22] << 3))                          & 0x07FF);
      
      // Failsafe flags tu byte 23
      sbus_frame_lost    = (sbus_buf[23] & 0x04) != 0;  // bit 2: frame lost
      sbus_failsafe_flag = (sbus_buf[23] & 0x08) != 0;  // bit 3: failsafe active
      
      sbus_last_frame_ms = millis();
      parse_errors = 0;  // Reset khi parse thành công
      return true;
    }
  }
  return false;
}

bool SBUS_failsafe() {
  return sbus_failsafe_flag || (millis() - sbus_last_frame_ms) > 100; 
}

static inline uint16_t get_rc(uint8_t ch) {
  if (ch < 1 || ch > 4) return 990;
  int x = sbus_ch_raw[ch - 1];
  if (x < 173) x = 173; if (x > 1811) x = 1811;
  return (uint16_t)((x - 173) * 1020 / 1638 + 990);
}

float SBUS_get_roll_target(float max_angle) {
  static float y = 0.0f;
  float n = ((float)get_rc(2) - 1500) / 500.0f;  // TX: Roll = CH2
  float target = (fabs(n * max_angle) < DEADZONE_ANGLE) ? 0.0f : n * max_angle;
  y = y * (1.0f - FILTER_ALPHA) + target * FILTER_ALPHA;
  return y;
}

float SBUS_get_pitch_target(float max_angle) {
  static float y = 0.0f;
  float n = ((float)get_rc(1) - 1500) / 500.0f;  // TX: Pitch = CH1
  float target = (fabs(n * max_angle) < DEADZONE_ANGLE) ? 0.0f : n * max_angle;
  y = y * (1.0f - FILTER_ALPHA) + target * FILTER_ALPHA;
  return y;
}

float SBUS_get_yaw_rate_target(float max_rate) {
  static float y = 0.0f;
  float n = ((float)get_rc(4) - 1500) / 500.0f;
  float target = (fabs(n * max_rate) < DEADZONE_YAW) ? 0.0f : n * max_rate;
  y = y * (1.0f - FILTER_ALPHA) + target * FILTER_ALPHA;
  return -y;  // Âm để đúng hướng
}

uint16_t SBUS_get_throttle(uint16_t max_throttle) {
  // Mapping throttle RC tiêu chuẩn: 990-2010 SBUS → 1000-2000 µs (tương thích Mamba F55)
  int y = (get_rc(3) - 990) * (max_throttle - 1000) / 1020 + 1000;
  if (y < 1000) return 1000;           // Throttle cực tiểu (tắt)
  if (y > max_throttle) return max_throttle;
  return y;
}

uint16_t SBUS_read_channel(uint8_t channel) {
  // Doc gia tri nguyen goc cua bat ky channel SBUS nao (0-15)
  // Tro ve gia tri theo µs (990-2010)
  if (channel > 15) return 990;  // Gia tri mac dinh neu channel khong hop le
  
  int x = sbus_ch_raw[channel];
  if (x < 173) x = 173;
  if (x > 1811) x = 1811;
  return (uint16_t)((x - 173) * 1020 / 1638 + 990);
}