#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

#define BUZZER_PIN  14
#define BUZZER_FREQ 2000
#define BUZZER_CH   6

inline void buzzer_init() {
  ledcSetup(BUZZER_CH, BUZZER_FREQ, 8);
  ledcAttachPin(BUZZER_PIN, BUZZER_CH);
  ledcWriteTone(BUZZER_CH, 0);
}

inline void beep(int ms = 50) {
  ledcWriteTone(BUZZER_CH, BUZZER_FREQ);
  delay(ms);
  ledcWriteTone(BUZZER_CH, 0);
}

#endif // BUZZER_H
