#include "PWM_Out.h"

#define ESC_PWM_FREQ 1000       // 1kHz - Tối ưu cho Mamba F55
#define ESC_PWM_RESOLUTION 11   // 11-bit = 2048 bước (phân giải 1.2µs)
#define ESC_MIN_PWM 1000        // Standard RC minimum (1000 µs)
#define ESC_MAX_PWM 2000        // Standard RC maximum (2000 µs)

static int esc_pins[4];

static inline int pwm_limit(int v) {
  if (v < ESC_MIN_PWM) return ESC_MIN_PWM;
  if (v > ESC_MAX_PWM) return ESC_MAX_PWM; 
  return v;
}

void PWM_setup_motor(int pin1, int pin2, int pin3, int pin4) {
  esc_pins[0] = pin1; esc_pins[1] = pin2; esc_pins[2] = pin3; esc_pins[3] = pin4;
  
  Serial.println("Cau hinh ESC: Mamba F55");
  Serial.printf("  Tan so: %d Hz\n", ESC_PWM_FREQ);
  Serial.printf("  Phan giai: %d-bit\n", ESC_PWM_RESOLUTION);
  Serial.printf("  Dai: %d-%d us\n", ESC_MIN_PWM, ESC_MAX_PWM);
  
  for(int i=0; i<4; i++){
    ledcSetup(i, ESC_PWM_FREQ, ESC_PWM_RESOLUTION);
    ledcAttachPin(esc_pins[i], i);
    ledcWrite(i, ESC_MIN_PWM);  // Gửi 1000µs - throttle min
  }
  Serial.println("ESC da gui tin hieu min (1000 us).");
}

void PWM_motor(int m1, int m2, int m3, int m4, bool arm) {
  if (arm) {
    ledcWrite(0, pwm_limit(m1)); 
    ledcWrite(1, pwm_limit(m2));
    ledcWrite(2, pwm_limit(m3)); 
    ledcWrite(3, pwm_limit(m4));
  } else {
    ledcWrite(0, ESC_MIN_PWM); 
    ledcWrite(1, ESC_MIN_PWM);
    ledcWrite(2, ESC_MIN_PWM); 
    ledcWrite(3, ESC_MIN_PWM);
  }
}

// Hàm trợ giúp để đặt tất cả động cơ với throttle giống nhau
void PWM_set_throttle(int throttle) {
  PWM_motor(throttle, throttle, throttle, throttle, throttle > ESC_MIN_PWM);
}