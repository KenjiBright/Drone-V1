# Drone V1 — ESP32 Flight Controller Firmware

Professional-grade quadcopter flight controller firmware for ESP32-based racing drones and FPV aircraft

---

## Features

### Core Flight Control
- **250Hz flight loop** on ESP32 Core 1 (dual-core FreeRTOS)
- **PID control** with advanced features:
  - D-term on measurement (derivative kick protection)
  - Integrator windup protection + output clamping
  - Tunable low-pass filter for gyro D-term
- **Kalman fusion** for accurate angle estimation from gyro + accelerometer
- **SBUS receiver** support (16 channels, 100kbaud, auto-inverted)
- **4-axis motor control** via ESC (1kHz PWM, 1000–2000µs range)

### Safety & Robustness
- **ARM switch** (AUX channel 5) for safe motor spinup
- **Automatic failsafe** on signal loss, IMU failure, or flip detection (>85° roll/pitch)
- **Dual-failsafe** detection: SBUS timeout (100ms) + receiver failsafe flag
- **Fault-tolerant** PID reset (transition-based, no redundant calculations)
- **Brownout protection** via firmware watchdog

### Data Logging & Tuning
- **Live Black Box logging** to Serial at 50Hz:
  - Setpoint + gyro rate + PID components (P, I, D per axis)
  - CSV format — import into Excel for analysis
- **Live PID tuning** via Serial menu (no recompile needed)
  - Adjust Kp/Ki/Kd/windup/output/D-filter on the fly
  - Copy Roll → Pitch with one command
- **Non-blocking Serial** menu system

### Hardware Integration
- **ICM20602 IMU** over SPI (4MHz, CS on GPIO 5)
- **WS2812B RGB LED** on GPIO 32 (status indicator)
- **Piezo buzzer** on GPIO 14 (feedback beeps)
- **EEPROM calibration** storage (gyro/accel offsets)

---

## Hardware Requirements

- **Microcontroller:** ESP32 (tested on ESP32-DevKitC)
- **IMU:** ICM20602 (SPI: CS=5, MOSI=23, MISO=19, SCK=18)
- **Motor ESCs:** 4× DSHOT600 or PWM (tested: Mamba F55)
- **SBUS Receiver:** Any FrSky/ELRS-compatible receiver (RX on GPIO 35)
- **Motor layout:** X-configuration (CCW front-left, CW front-right, CW rear-right, CCW rear-left)
- **Power:** 3S–4S LiPo (servo logic 5V, ESP32 via regulator)

### Pin Assignment

| GPIO | Function | Purpose |
|---|---|---|
| 5 | SPI CS | ICM20602 chip select |
| 14 | LEDC ch6 | Buzzer (2kHz) |
| 16 *old* → **35** | Serial2 RX | SBUS receiver |
| 18 | SPI SCK | IMU clock |
| 19 | SPI MISO | IMU data in |
| 23 | SPI MOSI | IMU data out |
| 25 | LEDC ch2 | Motor RR (PWM) |
| 26 | LEDC ch1 | Motor FR (PWM) |
| 27 | LEDC ch0 | Motor FL (PWM) |
| 32 | LEDC ch5 | WS2812B LED |
| 33 | LEDC ch3 | Motor RL (PWM) |

---

## Setup & Build

### Prerequisites
- **PlatformIO** (VSCode + PlatformIO extension)
- Board: `esp32dev` (ESP32-DevKitC)
- Framework: Arduino

### Build & Upload

```bash
# Clone repository
git clone <repo-url>
cd "Drone V1"

# Build
platformio run

# Upload to ESP32 (auto-detect COM port)
platformio run --target upload

# Monitor Serial (115200 baud)
platformio device monitor -- --baud 115200
```

### First-Time Setup

1. **Connect hardware** per pin assignments above
2. **Upload firmware**
3. **Open Serial Monitor** (115200 baud)
4. **Boot sequence:**
   ```
   [STARTUP] DRONE V1 Flight Controller
   [IMU] ICM20602 connected
   [ESC] Mamba F55 configured at 1kHz
   [Buzzer] 2x beep (ready)
   >> MENU PRINCIPAL (1=Log, 2=Calib, 3=PID Tune)
   ```

---

## Quick Reference

### Serial Menu

**Main Menu (press number + Enter):**
- `1` — Start/stop Black Box CSV log
- `2` — Calibration menu (gyro, accel, ESC)
- `3` — Live PID tuning
- `E` — Exit any mode

**PID Tuning Commands:**
```
rp 4.50    # Roll Kp = 4.50
ri 0.08    # Roll Ki = 0.08
rd 0.15    # Roll Kd = 0.15
rw 200     # Roll I-windup = 200
ro 400     # Roll output clamp = 400
rf 0.005   # Roll D-filter Tf = 0.005s
cp         # Copy Roll → Pitch (all params)
E          # Exit
```

### TX Configuration (Mode 2)

```
CH1 (Roll)     ← → Right stick ngang
CH2 (Pitch)    ↑ ↓ Right stick dọc
CH3 (Throttle) ↑ ↓ Left stick dọc
CH4 (Yaw)      ← → Left stick ngang
CH5 (ARM/LED)  2-pos switch (>1500µs = ARM)
```

See [docs/TX_SETUP.md](docs/TX_SETUP.md) for detailed TX setup.

---

## Documentation

- [**OVERVIEW.md**](docs/OVERVIEW.md) — Hardware, boot sequence, architecture
- [**TX_SETUP.md**](docs/TX_SETUP.md) — RC transmitter configuration (Mode 2, channel mapping, ARM switch)
- [**TX_SETUP.md**](docs/TX_SETUP.md) — RC transmitter configuration (Mode 2, channel mapping, ARM switch)
- [**CALIBRATION.md**](docs/CALIBRATION.md) — Calibration procedures (gyro, accel, ESC)
- [**BLACKBOX.md**](docs/BLACKBOX.md) — CSV logging format, Excel analysis
- [**PID_TUNING.md**](docs/PID_TUNING.md) — Live tuning, Kp/Ki/Kd guidelines, tuning workflow

### Tools & Utilities

**Python Black Box Capture & Analysis** → `tools/` directory

```bash
# Install dependencies
pip install -r tools/requirements.txt

# Capture 30 seconds of flight data with automatic analysis
python tools/capture_blackbox.py --port COM17 --duration 30 --desc "test1"

# Analyze existing CSV file
python tools/capture_blackbox.py --analyze flight_20260412_143630_test1.csv

# Windows quick-start
tools\capture.bat
```

**Generates:**
- **flight_YYYYMMDD_HHMMSS_description.csv** — Raw 50Hz sensor + PID data
- **flight_YYYYMMDD_HHMMSS_description.png** — 6-subplot analysis chart (Roll/Pitch/Yaw response + components)
- **Statistics** — RMS error, max error, throttle range per flight

**Use cases:**
- Verify PID tuning quality (setpoint tracking)
- Diagnose oscillations (P/I/D component visualization)
- Compare before/after tuning changes
- Archive flight characteristics for analysis

See [tools/README.md](tools/README.md) for full usage guide.

### Tools

**Python Black Box Capture & Analysis** (in `tools/` directory)

```bash
# Install dependencies
pip install -r tools/requirements.txt

# Capture 30 seconds of flight data
python tools/capture_blackbox.py --port COM17 --duration 30 --desc "test1"

# Analyze existing CSV file
python tools/capture_blackbox.py --analyze flight_20260412_143630_test1.csv
```

**Features:**
- Real-time capture from Serial port (50Hz Black Box data)
- Automatic CSV parsing and error metric calculation (RMS, max)
- Matplotlib visualization (6 subplots: Roll/Pitch/Yaw response + PID components)
- Auto-exports PNG chart with timestamps
- Windows/Linux/Mac support

**Quick Start:**
- Windows: Double-click `tools/capture.bat`
- Linux/Mac: `chmod +x tools/capture.sh && ./capture.sh`
- Details: See [tools/README.md](tools/README.md) and [tools/EXAMPLE_USAGE.md](tools/EXAMPLE_USAGE.md)

---

## 🔍 Data Analysis

After each flight session, use the Python tools to capture and visualize PID performance:

```bash
# Real-time capture from drone (generates CSV + PNG chart)
python tools/capture_blackbox.py --port COM17 --duration 30 --desc "hover_test"
```

**Output:**
- CSV with Setpoint/Gyro/PID components per axis at 50Hz
- PNG chart showing response curve + component breakdown
- Metrics: RMS error (°), max error, throttle range

**Analysis tips:**
- If `roll_gyro` lags `roll_sp` → increase Kp
- If `roll_gyro` overshoots `roll_sp` → increase Kd or decrease Kp
- If motor oscillates → reduce Kp/Kd or increase D-filter Tf
- Cross-check CSV against PNG for anomalies (glitches, clipping)

---

## Safety 

1. **Remove propellers** during initial testing (TX setup, calibration)
2. **Arm switch mandatory** — use AUX CH5 to prevent accidental spinup
3. **Failsafe enabled** — receiver will cut throttle on signal loss
4. **Calibrate gyro** before first flight (Menu → 2 → 1)
5. **Start with conservative PID** (defaults: Kp=4.5, Ki=0.08, Kd=0.15)
6. **Test in stabilize mode** first (don't fly in acro until tuned)
7. **Battery check** — ensure voltage above 3.0V per cell during hover

---

## Configuration Constants

Edit `src/main.cpp` to customize:

```cpp
#define MAX_ROLL_ANGLE   30.0f   // ±30°
#define MAX_PITCH_ANGLE  30.0f   // ±30°
#define MAX_YAW_RATE     50.0f   // ±50°/s
#define MAX_THROTTLE     2000    // PWM microseconds
#define MIN_THROTTLE     1000    // PWM microseconds
#define THROTTLE_MARGIN  50      // Throttle threshold above MIN for arm
#define LOOP_FREQ_HZ     250     // Flight loop frequency
#define FLIP_THRESHOLD   85.0f   // Auto-disarm if pitch/roll > 85°
```

---

## Troubleshooting

| Issue | Diagnosis | Fix |
|---|---|---|
| Motor doesn't respond | Check SBUS parse on Black Box log | Verify TX channel mapping (Ch1–4) |
| Drone wobbles | PID too aggressive | Reduce Kp by 0.5, retune |
| Slow to respond | Kp too low | Increase Kp by 0.5 |
| Motor oscillates | D-term too high | Reduce Kd by 0.05 or increase D-filter Tf |
| Drifts on one side | Gyro offset not calibrated | Menu → 2 → 1 (gyro calib) |
| Won't arm | ARM switch not detected | Check Ch5 in Black Box (need >1500µs) |
| Lost signal → motor stays on | Failsafe not configured | Set TX/RX failsafe to "No Pulses" |

---

## Architecture

### Dual-Core Design

**Core 0 (Priority 1, non-critical):**
- SBUS receiver polling (5ms tick)
- LED status update
- Serial menu handler
- Calibration state machine

**Core 1 (Priority 2, hard-real-time):**
- **250Hz flight loop** (4ms period)
- IMU sensor reads
- Kalman fusion
- PID computation
- Motor mixer
- Black Box logging

### Code Organization

```
src/
  ├── main.cpp              # Flight control loop + menu
  ├── ICM20602_IMU.cpp      # IMU driver + Kalman filter
  ├── SBUS_Receiver.cpp     # 16-channel SBUS parser
  ├── PWM_Out.cpp           # Motor ESC PWM control
  ├── Calibration.cpp       # Gyro/accel/ESC calibration
  ├── LED_Control.cpp       # WS2812B status LED
  ├── BlackBox.cpp          # CSV logging to Serial
  └── <others>

include/
  ├── SimplePID.h           # PID + D-filter + I-windup
  ├── Kalman1D.h            # 1D Kalman filter
  └── <headers>
```

---

## Recent Changes (v1.0)

### Bug Fixes
- Fixed Kalman filter `dt` — now uses actual sensor loop dt instead of hardcoded 0.005s
- Fixed SBUS parsing — now parses all 16 channels (was only 4)
- Fixed D-term spike on arm — seed `prev_measured` on first compute after reset
- Fixed PID reset logic — reset once on disarm transition, not every frame
- Fixed SBUS failsafe — check RX failsafe flag + timeout

### New Features
- ARM switch on AUX CH5 (safe motor spinup)
- LED/ARM state indication via same channel
- Live PID tuning commands (rp/ri/rd/rw/ro/rf/cp)
- Non-blocking Serial menu
- Black Box CSV logging at 50Hz

---

## 📄 License & Attribution

**License:** MIT (see LICENSE file)

Built with:
- Arduino framework
- PlatformIO
- ICM20602 SPI driver
- FreeRTOS (dual-core)
- SimplePID controller
- Custom Kalman fusion

---

### Recommended Improvements for Future Versions
- [ ] IMU gyro rate (currently ±500dps) → consider adding dynamic range select
- [ ] Acro mode (angle rate instead of angle target)
- [ ] Rate expo (stick response curve)
- [ ] DSHOT telemetry (ESC RPM feedback)
- [ ] Barometer altitude hold
- [ ] GPS positioning
- [ ] OSD (on-screen display) rendering

---

**Last Updated:** April 12, 2026  
**Firmware Version:** 1.0.0  
**Tools Version:** 1.0.0  
**Target Hardware:** ESP32 (esp32dev)  
**Repository:** https://github.com/KenjiBright/Drone-V1
# Drone V1 — ESP32 Flight Controller Firmware

Professional-grade quadcopter flight controller firmware for ESP32-based racing drones and FPV aircraft

---

## Features

### Core Flight Control
- **250Hz flight loop** on ESP32 Core 1 (dual-core FreeRTOS)
- **PID control** with advanced features:
  - D-term on measurement (derivative kick protection)
  - Integrator windup protection + output clamping
  - Tunable low-pass filter for gyro D-term
- **Kalman fusion** for accurate angle estimation from gyro + accelerometer
- **SBUS receiver** support (16 channels, 100kbaud, auto-inverted)
- **4-axis motor control** via ESC (1kHz PWM, 1000–2000µs range)

### Safety & Robustness
- **ARM switch** (AUX channel 5) for safe motor spinup
- **Automatic failsafe** on signal loss, IMU failure, or flip detection (>85° roll/pitch)
- **Dual-failsafe** detection: SBUS timeout (100ms) + receiver failsafe flag
- **Fault-tolerant** PID reset (transition-based, no redundant calculations)
- **Brownout protection** via firmware watchdog

### Data Logging & Tuning
- **Live Black Box logging** to Serial at 50Hz:
  - Setpoint + gyro rate + PID components (P, I, D per axis)
  - CSV format — import into Excel for analysis
- **Live PID tuning** via Serial menu (no recompile needed)
  - Adjust Kp/Ki/Kd/windup/output/D-filter on the fly
  - Copy Roll → Pitch with one command
- **Non-blocking Serial** menu system

### Hardware Integration
- **ICM20602 IMU** over SPI (4MHz, CS on GPIO 5)
- **WS2812B RGB LED** on GPIO 32 (status indicator)
- **Piezo buzzer** on GPIO 14 (feedback beeps)
- **EEPROM calibration** storage (gyro/accel offsets)

---

## Hardware Requirements

- **Microcontroller:** ESP32 (tested on ESP32-DevKitC)
- **IMU:** ICM20602 (SPI: CS=5, MOSI=23, MISO=19, SCK=18)
- **Motor ESCs:** 4× DSHOT600 or PWM (tested: Mamba F55)
- **SBUS Receiver:** Any FrSky/ELRS-compatible receiver (RX on GPIO 35)
- **Motor layout:** X-configuration (CCW front-left, CW front-right, CW rear-right, CCW rear-left)
- **Power:** 3S–4S LiPo (servo logic 5V, ESP32 via regulator)

### Pin Assignment

| GPIO | Function | Purpose |
|---|---|---|
| 5 | SPI CS | ICM20602 chip select |
| 14 | LEDC ch6 | Buzzer (2kHz) |
| 16 *old* → **35** | Serial2 RX | SBUS receiver |
| 18 | SPI SCK | IMU clock |
| 19 | SPI MISO | IMU data in |
| 23 | SPI MOSI | IMU data out |
| 25 | LEDC ch2 | Motor RR (PWM) |
| 26 | LEDC ch1 | Motor FR (PWM) |
| 27 | LEDC ch0 | Motor FL (PWM) |
| 32 | LEDC ch5 | WS2812B LED |
| 33 | LEDC ch3 | Motor RL (PWM) |

---

## Setup & Build

### Prerequisites
- **PlatformIO** (VSCode + PlatformIO extension)
- Board: `esp32dev` (ESP32-DevKitC)
- Framework: Arduino

### Build & Upload

```bash
# Clone repository
git clone <repo-url>
cd "Drone V1"

# Build
platformio run

# Upload to ESP32 (auto-detect COM port)
platformio run --target upload

# Monitor Serial (115200 baud)
platformio device monitor -- --baud 115200
```

### First-Time Setup

1. **Connect hardware** per pin assignments above
2. **Upload firmware**
3. **Open Serial Monitor** (115200 baud)
4. **Boot sequence:**
   ```
   [STARTUP] DRONE V1 Flight Controller
   [IMU] ICM20602 connected
   [ESC] Mamba F55 configured at 1kHz
   [Buzzer] 2x beep (ready)
   >> MENU PRINCIPAL (1=Log, 2=Calib, 3=PID Tune)
   ```

---

## Quick Reference

### Serial Menu

**Main Menu (press number + Enter):**
- `1` — Start/stop Black Box CSV log
- `2` — Calibration menu (gyro, accel, ESC)
- `3` — Live PID tuning
- `E` — Exit any mode

**PID Tuning Commands:**
```
rp 4.50    # Roll Kp = 4.50
ri 0.08    # Roll Ki = 0.08
rd 0.15    # Roll Kd = 0.15
rw 200     # Roll I-windup = 200
ro 400     # Roll output clamp = 400
rf 0.005   # Roll D-filter Tf = 0.005s
cp         # Copy Roll → Pitch (all params)
E          # Exit
```

### TX Configuration (Mode 2)

```
CH1 (Roll)     ← → Right stick ngang
CH2 (Pitch)    ↑ ↓ Right stick dọc
CH3 (Throttle) ↑ ↓ Left stick dọc
CH4 (Yaw)      ← → Left stick ngang
CH5 (ARM/LED)  2-pos switch (>1500µs = ARM)
```

See [docs/TX_SETUP.md](docs/TX_SETUP.md) for detailed TX setup.

---

## Documentation

- [**OVERVIEW.md**](docs/OVERVIEW.md) — Hardware, boot sequence, architecture
- [**TX_SETUP.md**](docs/TX_SETUP.md) — RC transmitter configuration (Mode 2, channel mapping, ARM switch)
- [**TX_SETUP.md**](docs/TX_SETUP.md) — RC transmitter configuration (Mode 2, channel mapping, ARM switch)
- [**CALIBRATION.md**](docs/CALIBRATION.md) — Calibration procedures (gyro, accel, ESC)
- [**BLACKBOX.md**](docs/BLACKBOX.md) — CSV logging format, Excel analysis
- [**PID_TUNING.md**](docs/PID_TUNING.md) — Live tuning, Kp/Ki/Kd guidelines, tuning workflow

### Tools & Utilities

**Python Black Box Capture & Analysis** → `tools/` directory

```bash
# Install dependencies
pip install -r tools/requirements.txt

# Capture 30 seconds of flight data with automatic analysis
python tools/capture_blackbox.py --port COM17 --duration 30 --desc "test1"

# Analyze existing CSV file
python tools/capture_blackbox.py --analyze flight_20260412_143630_test1.csv

# Windows quick-start
tools\capture.bat
```

**Generates:**
- **flight_YYYYMMDD_HHMMSS_description.csv** — Raw 50Hz sensor + PID data
- **flight_YYYYMMDD_HHMMSS_description.png** — 6-subplot analysis chart (Roll/Pitch/Yaw response + components)
- **Statistics** — RMS error, max error, throttle range per flight

**Use cases:**
- Verify PID tuning quality (setpoint tracking)
- Diagnose oscillations (P/I/D component visualization)
- Compare before/after tuning changes
- Archive flight characteristics for analysis

See [tools/README.md](tools/README.md) for full usage guide.

### Tools

**Python Black Box Capture & Analysis** (in `tools/` directory)

```bash
# Install dependencies
pip install -r tools/requirements.txt

# Capture 30 seconds of flight data
python tools/capture_blackbox.py --port COM17 --duration 30 --desc "test1"

# Analyze existing CSV file
python tools/capture_blackbox.py --analyze flight_20260412_143630_test1.csv
```

**Features:**
- Real-time capture from Serial port (50Hz Black Box data)
- Automatic CSV parsing and error metric calculation (RMS, max)
- Matplotlib visualization (6 subplots: Roll/Pitch/Yaw response + PID components)
- Auto-exports PNG chart with timestamps
- Windows/Linux/Mac support

**Quick Start:**
- Windows: Double-click `tools/capture.bat`
- Linux/Mac: `chmod +x tools/capture.sh && ./capture.sh`
- Details: See [tools/README.md](tools/README.md) and [tools/EXAMPLE_USAGE.md](tools/EXAMPLE_USAGE.md)

---

## 🔍 Data Analysis

After each flight session, use the Python tools to capture and visualize PID performance:

```bash
# Real-time capture from drone (generates CSV + PNG chart)
python tools/capture_blackbox.py --port COM17 --duration 30 --desc "hover_test"
```

**Output:**
- CSV with Setpoint/Gyro/PID components per axis at 50Hz
- PNG chart showing response curve + component breakdown
- Metrics: RMS error (°), max error, throttle range

**Analysis tips:**
- If `roll_gyro` lags `roll_sp` → increase Kp
- If `roll_gyro` overshoots `roll_sp` → increase Kd or decrease Kp
- If motor oscillates → reduce Kp/Kd or increase D-filter Tf
- Cross-check CSV against PNG for anomalies (glitches, clipping)

---

## Safety 

1. **Remove propellers** during initial testing (TX setup, calibration)
2. **Arm switch mandatory** — use AUX CH5 to prevent accidental spinup
3. **Failsafe enabled** — receiver will cut throttle on signal loss
4. **Calibrate gyro** before first flight (Menu → 2 → 1)
5. **Start with conservative PID** (defaults: Kp=4.5, Ki=0.08, Kd=0.15)
6. **Test in stabilize mode** first (don't fly in acro until tuned)
7. **Battery check** — ensure voltage above 3.0V per cell during hover

---

## Configuration Constants

Edit `src/main.cpp` to customize:

```cpp
#define MAX_ROLL_ANGLE   30.0f   // ±30°
#define MAX_PITCH_ANGLE  30.0f   // ±30°
#define MAX_YAW_RATE     50.0f   // ±50°/s
#define MAX_THROTTLE     2000    // PWM microseconds
#define MIN_THROTTLE     1000    // PWM microseconds
#define THROTTLE_MARGIN  50      // Throttle threshold above MIN for arm
#define LOOP_FREQ_HZ     250     // Flight loop frequency
#define FLIP_THRESHOLD   85.0f   // Auto-disarm if pitch/roll > 85°
```

---

## Troubleshooting

| Issue | Diagnosis | Fix |
|---|---|---|
| Motor doesn't respond | Check SBUS parse on Black Box log | Verify TX channel mapping (Ch1–4) |
| Drone wobbles | PID too aggressive | Reduce Kp by 0.5, retune |
| Slow to respond | Kp too low | Increase Kp by 0.5 |
| Motor oscillates | D-term too high | Reduce Kd by 0.05 or increase D-filter Tf |
| Drifts on one side | Gyro offset not calibrated | Menu → 2 → 1 (gyro calib) |
| Won't arm | ARM switch not detected | Check Ch5 in Black Box (need >1500µs) |
| Lost signal → motor stays on | Failsafe not configured | Set TX/RX failsafe to "No Pulses" |

---

## Architecture

### Dual-Core Design

**Core 0 (Priority 1, non-critical):**
- SBUS receiver polling (5ms tick)
- LED status update
- Serial menu handler
- Calibration state machine

**Core 1 (Priority 2, hard-real-time):**
- **250Hz flight loop** (4ms period)
- IMU sensor reads
- Kalman fusion
- PID computation
- Motor mixer
- Black Box logging

### Code Organization

```
src/
  ├── main.cpp              # Flight control loop + menu
  ├── ICM20602_IMU.cpp      # IMU driver + Kalman filter
  ├── SBUS_Receiver.cpp     # 16-channel SBUS parser
  ├── PWM_Out.cpp           # Motor ESC PWM control
  ├── Calibration.cpp       # Gyro/accel/ESC calibration
  ├── LED_Control.cpp       # WS2812B status LED
  ├── BlackBox.cpp          # CSV logging to Serial
  └── <others>

include/
  ├── SimplePID.h           # PID + D-filter + I-windup
  ├── Kalman1D.h            # 1D Kalman filter
  └── <headers>
```

---

## Recent Changes (v1.0)

### Bug Fixes
- Fixed Kalman filter `dt` — now uses actual sensor loop dt instead of hardcoded 0.005s
- Fixed SBUS parsing — now parses all 16 channels (was only 4)
- Fixed D-term spike on arm — seed `prev_measured` on first compute after reset
- Fixed PID reset logic — reset once on disarm transition, not every frame
- Fixed SBUS failsafe — check RX failsafe flag + timeout

### New Features
- ARM switch on AUX CH5 (safe motor spinup)
- LED/ARM state indication via same channel
- Live PID tuning commands (rp/ri/rd/rw/ro/rf/cp)
- Non-blocking Serial menu
- Black Box CSV logging at 50Hz

---

## 📄 License & Attribution

**License:** MIT (see LICENSE file)

Built with:
- Arduino framework
- PlatformIO
- ICM20602 SPI driver
- FreeRTOS (dual-core)
- SimplePID controller
- Custom Kalman fusion

---

### Recommended Improvements for Future Versions
- [ ] IMU gyro rate (currently ±500dps) → consider adding dynamic range select
- [ ] Acro mode (angle rate instead of angle target)
- [ ] Rate expo (stick response curve)
- [ ] DSHOT telemetry (ESC RPM feedback)
- [ ] Barometer altitude hold
- [ ] GPS positioning
- [ ] OSD (on-screen display) rendering

---

**Last Updated:** April 12, 2026  
**Firmware Version:** 1.0.0  
**Tools Version:** 1.0.0  
**Target Hardware:** ESP32 (esp32dev)  
**Repository:** https://github.com/KenjiBright/Drone-V1
