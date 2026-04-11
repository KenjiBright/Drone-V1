#include "LED_Control.h"
#include "SBUS_Receiver.h"

static bool led_state = false;
static uint32_t last_update = 0;

void LED_setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);  // Tat LED ban dau
  led_state = false;
  Serial.printf("[LED] Setup GPIO %d thanh cong\n", LED_PIN);
}

void LED_update() {
  // Chi cap nhat neu co du lieu SBUS moi (tranh cap nhat qua nhanh)
  if (millis() - last_update < 50) return;
  last_update = millis();
  
  // Doc gia tri pin dien ap tu SBUS channel 5
  // SBUS channel 5 se o trong sbus_ch_raw[4] (chi so 0-based)
  // Hiện tại SBUS_Receiver chi parse 4 channels dau, nen ta dung channel 4 lam AUX
  uint16_t channel_value = SBUS_read_channel(4);  // Channel 4 (chi so 0-based, ~ channel 5)
  
  bool new_state = (channel_value > LED_THRESHOLD);
  
  if (new_state != led_state) {
    led_state = new_state;
    digitalWrite(LED_PIN, led_state ? HIGH : LOW);
    Serial.printf("[LED] %s (Channel value: %d)\n", 
                  led_state ? "ON" : "OFF", channel_value);
  }
}

void LED_set(bool state) {
  led_state = state;
  digitalWrite(LED_PIN, state ? HIGH : LOW);
  Serial.printf("[LED] Dat trang thai: %s\n", state ? "ON" : "OFF");
}

bool LED_get() {
  return led_state;
}

void LED_toggle() {
  LED_set(!led_state);
}
