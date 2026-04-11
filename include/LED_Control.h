#ifndef LED_CONTROL_H
#define LED_CONTROL_H
#include <Arduino.h>

#define LED_PIN 32              // GPIO 32
#define LED_CHANNEL 5           // SBUS Channel 5 (kênh 5 để điều khiển LED)
#define LED_THRESHOLD 1500      // Ngưỡng: >1500µs = LED ON, <1500µs = LED OFF

// Khởi tạo LED
void LED_setup();

// Cập nhật trạng thái LED dựa trên kênh SBUS
void LED_update();

// Đặt trạng thái LED trực tiếp
void LED_set(bool state);

// Lấy trạng thái LED hiện tại
bool LED_get();

// Toggle LED
void LED_toggle();

#endif
