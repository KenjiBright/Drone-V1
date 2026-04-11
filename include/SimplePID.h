#ifndef SIMPLE_PID_H
#define SIMPLE_PID_H

struct SimplePID {
  float Kp, Ki, Kd;

  // Gioi han
  float max_integral = 200.0f;  // Windup limit
  float max_output   = 400.0f;  // Clamp tong output

  // D-term low-pass filter: alpha = Tf/(Tf+dt)
  // alpha = 0 -> khong loc  alpha -> 1 -> loc manh
  // Tf ~ 0.005 -> alpha ~0.55 o 250Hz (dt=0.004s)
  float d_lpf_tf = 0.005f;      // Hang so thoi gian filter (giac)

  // Trang thai noi bo
  float integral       = 0;
  float prev_measured  = 0;     // D tinh tren measured (tranh derivative kick)
  float d_filtered     = 0;     // Gia tri D sau LPF
  bool  first_run      = true;  // Seed prev_measured lan dau

  // Luu thanh phan P/I/D cuoi de log BlackBox
  float last_p = 0;
  float last_i = 0;
  float last_d = 0;

  SimplePID(float p, float i, float d) : Kp(p), Ki(i), Kd(d) {}

  float compute(float setpoint, float measured, float dt) {
    float error = setpoint - measured;

    // P
    last_p = Kp * error;

    // I voi windup clamp
    integral += error * dt;
    if      (integral >  max_integral) integral =  max_integral;
    else if (integral < -max_integral) integral = -max_integral;
    last_i = Ki * integral;

    // D tren measured (tranh derivative kick khi doi setpoint dot ngot)
    // + low-pass filter de giam nhieu gyro
    if (first_run) { prev_measured = measured; first_run = false; }
    float d_raw = -(measured - prev_measured) / dt;  // am vi do tren measured
    prev_measured = measured;
    float alpha  = d_lpf_tf / (d_lpf_tf + dt);
    d_filtered   = alpha * d_filtered + (1.0f - alpha) * d_raw;
    last_d = Kd * d_filtered;

    // Tong output co clamp
    float output = last_p + last_i + last_d;
    if      (output >  max_output) output =  max_output;
    else if (output < -max_output) output = -max_output;
    return output;
  }

  void reset() {
    integral      = 0;
    prev_measured = 0;
    d_filtered    = 0;
    first_run     = true;
    last_p = last_i = last_d = 0;
  }
};
#endif
