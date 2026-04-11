#ifndef PWM_OUT_H
#define PWM_OUT_H
#include <Arduino.h>

void PWM_setup_motor(int pin1, int pin2, int pin3, int pin4);
void PWM_motor(int m1, int m2, int m3, int m4, bool arm);
void PWM_set_throttle(int throttle);
#endif