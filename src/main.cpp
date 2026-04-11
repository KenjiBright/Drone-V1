#include <Arduino.h>
#include "ICM20602_IMU.h"
#include "SimplePID.h"
#include "PWM_Out.h"
#include "SBUS_Receiver.h"
#include "Calibration.h"
#include "LED_Control.h"
#include "SPIFFS_Logger.h"
#include "Buzzer.h"

// ===== CẤU HÌNH =====
#define MAX_ROLL_ANGLE   30.0f
#define MAX_PITCH_ANGLE  30.0f
#define MAX_YAW_RATE     50.0f
#define MAX_THROTTLE     2000
#define THROTTLE_MARGIN  50
#define MIN_THROTTLE     1000

#define LOOP_FREQ_HZ     250
#define LOOP_TIME_US     (1000000 / LOOP_FREQ_HZ)
#define FLIP_THRESHOLD   85.0f

// ===== PID =====
SimplePID pid_roll(4.5, 0.08, 0.15);
SimplePID pid_pitch(4.5, 0.08, 0.15);
SimplePID pid_yaw(2.5, 0.05, 0.0);

// ===== SHARED STATE (Core 0 <-> Core 1) =====
volatile float target_roll     = 0.0f;
volatile float target_pitch    = 0.0f;
volatile float target_yaw_rate = 0.0f;
volatile int   throttle        = MIN_THROTTLE;
volatile bool  failsafe_active = true;
volatile bool  arm_switch      = false;   // AUX ch5 arm switch

// Trang thai arm truoc do (Core1 dung de detect transition)
static bool was_armed = false;

TaskHandle_t TaskCore0_Handle;
TaskHandle_t TaskCore1_Handle;

// ===== MENU STATE =====
enum MenuMode { MODE_MAIN, MODE_LOG, MODE_CALIB, MODE_PID, MODE_EXPORT };
static MenuMode menu_mode    = MODE_MAIN;
static String   serial_buf   = "";


// =========================================================
// MENU FUNCTIONS
// =========================================================

void print_main_menu() {
  Serial.println("\n==============================");
  Serial.println("         MENU CHINH           ");
  Serial.println("==============================");
  Serial.println(" 1) Black Box Log (SPIFFS)");
  Serial.println(" 2) Calibration");
  Serial.println(" 3) PID Tuning (Live)");
  Serial.println(" 4) List / Export Black Box Files");
  Serial.println("  >> Nhan E de thoat bat ky mode nao");
  Serial.println("==============================\n");
}

void exit_current_mode() {
  if (menu_mode == MODE_LOG) {
    SPIFFS_stopLog();
  }
  menu_mode = MODE_MAIN;
  serial_buf = "";
  beep();
  Serial.println();
  print_main_menu();
}

void handle_pid_command(const String& line) {
  // Format: axis + param + space + value
  // Params: p=Kp  i=Ki  d=Kd  w=max_integral  o=max_output  f=d_lpf_tf
  // Lenh dac biet: cp -> copy tat ca Roll sang Pitch
  // Vi du:  rp 4.50   rd 0.15   rw 200   ro 400   rf 0.005   cp

  // Lenh dac biet: cp
  if (line == "cp" || line == "CP") {
    pid_pitch.Kp           = pid_roll.Kp;
    pid_pitch.Ki           = pid_roll.Ki;
    pid_pitch.Kd           = pid_roll.Kd;
    pid_pitch.max_integral = pid_roll.max_integral;
    pid_pitch.max_output   = pid_roll.max_output;
    pid_pitch.d_lpf_tf     = pid_roll.d_lpf_tf;
    beep();
    Serial.println("[PID] Da copy Roll -> Pitch:");
    Serial.printf(
      "[PID] Roll:  Kp=%.3f Ki=%.4f Kd=%.4f | windup=%.0f out=%.0f Tf=%.4f\n"
      "[PID] Pitch: Kp=%.3f Ki=%.4f Kd=%.4f | windup=%.0f out=%.0f Tf=%.4f\n",
      pid_roll.Kp,  pid_roll.Ki,  pid_roll.Kd,  pid_roll.max_integral,  pid_roll.max_output,  pid_roll.d_lpf_tf,
      pid_pitch.Kp, pid_pitch.Ki, pid_pitch.Kd, pid_pitch.max_integral, pid_pitch.max_output, pid_pitch.d_lpf_tf
    );
    return;
  }

  if (line.length() < 4) {
    Serial.println("[PID] Lenh khong dung. Vi du: rp 4.50  rd 0.15  rw 200  ro 400  rf 0.005  cp");
    return;
  }
  char axis  = tolower((char)line[0]);
  char param = tolower((char)line[1]);
  float val  = line.substring(3).toFloat();

  SimplePID*  pid       = nullptr;
  const char* axis_name = "";
  if      (axis == 'r') { pid = &pid_roll;  axis_name = "Roll"; }
  else if (axis == 'p') { pid = &pid_pitch; axis_name = "Pitch"; }
  else if (axis == 'y') { pid = &pid_yaw;   axis_name = "Yaw"; }
  else {
    Serial.println("[PID] Truc khong hop le (r=Roll, p=Pitch, y=Yaw)");
    return;
  }

  switch (param) {
    case 'p': pid->Kp           = val; beep(); Serial.printf("[PID] %s Kp = %.4f\n",          axis_name, val); break;
    case 'i': pid->Ki           = val; beep(); Serial.printf("[PID] %s Ki = %.4f\n",          axis_name, val); break;
    case 'd': pid->Kd           = val; beep(); Serial.printf("[PID] %s Kd = %.4f\n",          axis_name, val); break;
    case 'w': pid->max_integral = val; beep(); Serial.printf("[PID] %s max_integral = %.1f\n", axis_name, val); break;
    case 'o': pid->max_output   = val; beep(); Serial.printf("[PID] %s max_output = %.1f\n",   axis_name, val); break;
    case 'f': pid->d_lpf_tf     = val; beep(); Serial.printf("[PID] %s D-LPF Tf = %.4f s\n",  axis_name, val); break;
    default:
      Serial.println("[PID] Tham so: p=Kp  i=Ki  d=Kd  w=windup  o=output_limit  f=D_filter");
      return;
  }
  Serial.printf(
    "[PID] Roll:  Kp=%.3f Ki=%.4f Kd=%.4f | windup=%.0f out=%.0f Tf=%.4f\n"
    "[PID] Pitch: Kp=%.3f Ki=%.4f Kd=%.4f | windup=%.0f out=%.0f Tf=%.4f\n"
    "[PID] Yaw:   Kp=%.3f Ki=%.4f Kd=%.4f | windup=%.0f out=%.0f Tf=%.4f\n",
    pid_roll.Kp,  pid_roll.Ki,  pid_roll.Kd,  pid_roll.max_integral,  pid_roll.max_output,  pid_roll.d_lpf_tf,
    pid_pitch.Kp, pid_pitch.Ki, pid_pitch.Kd, pid_pitch.max_integral, pid_pitch.max_output, pid_pitch.d_lpf_tf,
    pid_yaw.Kp,   pid_yaw.Ki,   pid_yaw.Kd,   pid_yaw.max_integral,   pid_yaw.max_output,   pid_yaw.d_lpf_tf
  );
}

void process_command(const String& cmd) {
  if (cmd.length() == 0) return;

  switch (menu_mode) {

    case MODE_MAIN:
      if      (cmd == "1") {
        menu_mode = MODE_LOG;
        beep();
        Serial.print("[SPIFFS] Log description (default=log): ");
      }
      else if (cmd == "2") {
        menu_mode = MODE_CALIB;
        beep();
        Calibration_display_menu();
        Serial.println("  >> Nhan E de thoat ve menu chinh.");
      }
      else if (cmd == "3") {
        menu_mode = MODE_PID;
        beep();
        Serial.println("\n[PID TUNING] Nhap lenh va Enter de cap nhat. E=Thoat");
        Serial.println("  Params: p=Kp  i=Ki  d=Kd  w=windup  o=output_limit  f=D_filter_Tf");
        Serial.println("  Vi du:  rp 4.50   rd 0.15   rw 200   ro 400   rf 0.005");
        Serial.printf(
          "  Roll:  Kp=%.3f Ki=%.4f Kd=%.4f | windup=%.0f out=%.0f Tf=%.4f\n"
          "  Pitch: Kp=%.3f Ki=%.4f Kd=%.4f | windup=%.0f out=%.0f Tf=%.4f\n"
          "  Yaw:   Kp=%.3f Ki=%.4f Kd=%.4f | windup=%.0f out=%.0f Tf=%.4f\n",
          pid_roll.Kp,  pid_roll.Ki,  pid_roll.Kd,  pid_roll.max_integral,  pid_roll.max_output,  pid_roll.d_lpf_tf,
          pid_pitch.Kp, pid_pitch.Ki, pid_pitch.Kd, pid_pitch.max_integral, pid_pitch.max_output, pid_pitch.d_lpf_tf,
          pid_yaw.Kp,   pid_yaw.Ki,   pid_yaw.Kd,   pid_yaw.max_integral,   pid_yaw.max_output,   pid_yaw.d_lpf_tf);
        Serial.println("---");
      }
      else if (cmd == "4") {
        menu_mode = MODE_EXPORT;
        beep();
        Serial.println("\n[SPIFFS] Available log files:");
        SPIFFS_listFiles();
        SPIFFS_printStats();
        Serial.print("[SPIFFS] Export filename (e.g., 20260412_143630_hover.csv) or ENTER to skip: ");
      }
      else {
        print_main_menu();
      }
      break;

    case MODE_LOG:
      // Start logging with description
      {
        String desc = cmd.length() > 0 ? cmd : "log";
        SPIFFS_startLog(desc.c_str());
        beep();
        Serial.println("[SPIFFS] LOGGING STARTED - Press E to stop");
        Serial.println("[SPIFFS] Returning to main menu (logging continues in background)");
        menu_mode = MODE_MAIN;
      }
      break;

    case MODE_CALIB: {
      char c = cmd[0];
      switch (c) {
        case '1': beep(); Calibration_imu_gyro_start(); break;
        case '2': Serial.println("Sap toi - Calib accel 6-diem thu cong"); break;
        case '3':
          beep(200);
          Serial.println("WARNING: NGAT KET NOI BATTERY TRUOC KHI TIEP TUC!");
          Calibration_esc_start();
          break;
        case '4':
          beep();
          Serial.println("Kiem tra toan dai ESC (1000-2000 us)");
          for (int v = 1000; v <= 2000; v += 100) {
            PWM_motor(v, v, v, v, true);
            Serial.printf("PWM: %d us\n", v);
            delay(500);
          }
          PWM_motor(1000, 1000, 1000, 1000, false);
          break;
        case '5': beep(); Calibration_load_from_eeprom(); break;
        case '6':
          Serial.println("Go 'RESET' va Enter de confirm:");
          break;
        case '7': Calibration_display_data(); break;
        default:
          if (cmd == "RESET") {
            beep(100); delay(80); beep(100); delay(80); beep(100);
            Calibration_reset_all();
          } else {
            Calibration_display_menu();
          }
      }
      break;
    }

    case MODE_PID:
      handle_pid_command(cmd);
      break;

    case MODE_EXPORT:
      if (cmd.length() == 0) {
        Serial.println("[SPIFFS] Skipped");
        menu_mode = MODE_MAIN;
        print_main_menu();
      } else {
        // Append .csv if not present
        String filename = cmd;
        if (!filename.endsWith(".csv")) {
          filename += ".csv";
        }
        // Export file contents
        SPIFFS_exportCSV(filename.c_str());
        beep();
        Serial.println("[SPIFFS] Returning to main menu");
        menu_mode = MODE_MAIN;
        print_main_menu();
      }
      break;
  }
}

// =========================================================
// NON-BLOCKING SERIAL HANDLER  (gọi từ TaskCore0)
// =========================================================

void handle_serial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r') continue;

    if (c == '\n') {
      serial_buf.trim();
      if (serial_buf == "E" || serial_buf == "e") {
        exit_current_mode();
      } else {
        process_command(serial_buf);
      }
      serial_buf = "";
      return;
    }

    // Phím đơn không cần Enter: E thoát bất kỳ mode
    if ((c == 'E' || c == 'e') && serial_buf.length() == 0) {
      exit_current_mode();
      return;
    }

    // Phím đơn không cần Enter: chọn menu chính (1/2/3/4)
    if (menu_mode == MODE_MAIN && serial_buf.length() == 0 &&
        (c == '1' || c == '2' || c == '3' || c == '4')) {
      String s(c);
      process_command(s);
      return;
    }

    serial_buf += c;
    if (serial_buf.length() > 64) serial_buf = "";
  }
}

// =========================================================
// CALIBRATION UPDATE  (gọi khi đang trong quá trình calib)
// =========================================================

void update_calibration() {
  if (Calibration_get_mode() == CALIB_IMU_GYRO) {
    Calibration_imu_gyro_update();
  } else if (Calibration_get_mode() == CALIB_ESC_MIN_MAX) {
    Calibration_esc_update();
  }
}

// =========================================================
// CORE 0: SBUS + LED + Calibration update + Serial Menu
// =========================================================

void TaskCore0(void *pvParameters) {
  for (;;) {
    if (SBUS_update()) {
      target_roll      = SBUS_get_roll_target(MAX_ROLL_ANGLE);
      target_pitch     = SBUS_get_pitch_target(MAX_PITCH_ANGLE);
      target_yaw_rate  = SBUS_get_yaw_rate_target(MAX_YAW_RATE);
      throttle         = SBUS_get_throttle(MAX_THROTTLE);
      // AUX ch5 (index 4): >1500 = ARM
      arm_switch       = (SBUS_read_channel(4) > 1500);
      failsafe_active  = false;
    }
    if (SBUS_failsafe()) {
      failsafe_active = true;
      arm_switch      = false;
      throttle = MIN_THROTTLE;
    }

    LED_update();
    update_calibration();
    handle_serial();

    vTaskDelay(5 / portTICK_PERIOD_MS);
  }
}

// =========================================================
// CORE 1: FLIGHT CONTROL LOOP  250Hz
// =========================================================

void TaskCore1(void *pvParameters) {
  uint32_t next_loop_time = micros();
  for (;;) {
    while (micros() < next_loop_time) { yield(); }
    uint32_t now = micros();
    float dt = (now - (next_loop_time - LOOP_TIME_US)) / 1000000.0f;
    next_loop_time += LOOP_TIME_US;

    IMU_update(dt);

    if (!IMU_is_healthy() ||
        fabs(IMU_get_angle_roll())  > FLIP_THRESHOLD ||
        fabs(IMU_get_angle_pitch()) > FLIP_THRESHOLD) {
      failsafe_active = true;
      throttle = MIN_THROTTLE;
    }

    // Dieu kien armed: arm_switch ON + throttle du + khong failsafe
    bool armed = arm_switch &&
                 (throttle > (MIN_THROTTLE + THROTTLE_MARGIN)) &&
                 !failsafe_active;

    // Detect disarm transition -> reset PID 1 lan
    if (was_armed && !armed) {
      pid_roll.reset(); pid_pitch.reset(); pid_yaw.reset();
    }
    was_armed = armed;

    float out_roll  = 0, out_pitch = 0, out_yaw = 0;
    if (armed) {
      out_roll  = pid_roll.compute(target_roll,      IMU_get_angle_roll(),  dt);
      out_pitch = pid_pitch.compute(target_pitch,    IMU_get_angle_pitch(), dt);
      out_yaw   = pid_yaw.compute(target_yaw_rate,   IMU_get_rate_yaw(),    dt);
    }

    //  Motor layout (X-Quad):
    //  m1 FL (CCW)  m2 FR (CW)  m3 RR (CW)  m4 RL (CCW)
    int m1 = throttle - out_pitch + out_roll - out_yaw;
    int m2 = throttle - out_pitch - out_roll + out_yaw;
    int m3 = throttle + out_pitch - out_roll + out_yaw;
    int m4 = throttle + out_pitch + out_roll - out_yaw;

    PWM_motor(m1, m2, m3, m4, armed);

    SPIFFS_log_data(
      throttle,
      target_roll,      IMU_get_rate_roll(),  pid_roll.last_p,  pid_roll.last_i,  pid_roll.last_d,
      target_pitch,     IMU_get_rate_pitch(), pid_pitch.last_p, pid_pitch.last_i, pid_pitch.last_d,
      target_yaw_rate,  IMU_get_rate_yaw(),   pid_yaw.last_p,   pid_yaw.last_i,   pid_yaw.last_d
    );
  }
}

// =========================================================
// SETUP
// =========================================================

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n========================================");
  Serial.println("  DRONE V1 FLIGHT CONTROLLER - Bat Dau");
  Serial.println("========================================");

  Calibration_init();
  PWM_setup_motor(27, 26, 25, 33);
  SBUS_begin(Serial2, 35);
  IMU_begin();
  IMU_calibrate();
  LED_setup();
  SPIFFS_init();

  // Khoi tao buzzer
  buzzer_init();
  beep(100); delay(80); beep(100);  // 2 tieng bip khi boot xong

  Serial.printf("PID Roll/Pitch : Kp=%.2f Ki=%.3f Kd=%.3f\n",
                pid_roll.Kp, pid_roll.Ki, pid_roll.Kd);
  Serial.printf("PID Yaw        : Kp=%.2f Ki=%.3f Kd=%.3f\n",
                pid_yaw.Kp, pid_yaw.Ki, pid_yaw.Kd);
  Serial.printf("Loop           : %d Hz\n", LOOP_FREQ_HZ);

  print_main_menu();

  xTaskCreatePinnedToCore(TaskCore0, "TaskCore0", 10000, NULL, 1, &TaskCore0_Handle, 0);
  xTaskCreatePinnedToCore(TaskCore1, "TaskCore1", 10000, NULL, 2, &TaskCore1_Handle, 1);
}

void loop() {
  vTaskDelete(NULL);
}


