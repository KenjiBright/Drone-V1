#ifndef SBUS_RECEIVER_H
#define SBUS_RECEIVER_H
#include <Arduino.h>

void SBUS_begin(HardwareSerial &port, int rxPin);
bool SBUS_update();
bool SBUS_failsafe(); 

float SBUS_get_roll_target(float max_angle);
float SBUS_get_pitch_target(float max_angle);
float SBUS_get_yaw_rate_target(float max_rate);
uint16_t SBUS_get_throttle(uint16_t max_throttle);
uint16_t SBUS_read_channel(uint8_t channel);  // Doc tín hiệu từ bất kỳ kênh nào (0-15) theo µs
#endif