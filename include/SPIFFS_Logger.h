#ifndef SPIFFS_LOGGER_H
#define SPIFFS_LOGGER_H

#include <Arduino.h>
#include <SPIFFS.h>

// ===== SPIFFS BLACK BOX LOGGER =====
// Ghi log dữ liệu bay vào SPIFFS (bộ nhớ flash nội bộ ESP32)
// Không xung đột port COM
// Định dạng: CSV ghi tuần tự vào file
//
// Sử dụng:
//   - SPIFFS_init() — khởi động khi boot
//   - SPIFFS_startLog() — bắt đầu ghi log trước bay
//   - SPIFFS_log_data() — ghi mỗi row (gọi trong flight loop)
//   - SPIFFS_stopLog() — dừng ghi log sau bay
//   - SPIFFS_exportCSV() — xuất CSV qua Serial
//   - SPIFFS_listFiles() — xem danh sách file log

#define SPIFFS_LOG_DIR "/"
#define SPIFFS_MAX_FILES 10
#define CSV_BUFFER_SIZE 256

typedef struct {
    bool    is_logging;
    File    current_file;
    char    filename[64];
    uint32_t bytes_written;
} SPIFFSLogger_t;

// Global logger state
extern SPIFFSLogger_t spiffs_logger;

/**
 * @brief Initialize SPIFFS filesystem
 */
void SPIFFS_init();

/**
 * @brief Start new log session (creates new file with timestamp)
 * @param description Optional description for filename (max 20 chars)
 */
void SPIFFS_startLog(const char *description = "log");

/**
 * @brief Stop current logging session (closes file)
 */
void SPIFFS_stopLog();

/**
 * @brief Write CSV row to current log file
 * Automatically adds timestamp and handles buffering
 */
void SPIFFS_log_data(
    int   throttle,
    // Roll
    float roll_sp,  float roll_gyro,  float roll_P,  float roll_I,  float roll_D,
    // Pitch
    float pitch_sp, float pitch_gyro, float pitch_P, float pitch_I, float pitch_D,
    // Yaw
    float yaw_sp,   float yaw_gyro,   float yaw_P,   float yaw_I,   float yaw_D
);

/**
 * @brief Export CSV file via Serial (human-readable output)
 * User should copy-paste into text editor and save as .csv
 * 
 * @param filename Filename to export (e.g., "20260412_143630_hover.csv")
 */
void SPIFFS_exportCSV(const char *filename);

/**
 * @brief List all log files on SPIFFS
 */
void SPIFFS_listFiles();

/**
 * @brief Delete specific log file
 * 
 * @param filename Filename to delete
 * @return true if deleted, false if not found
 */
bool SPIFFS_deleteFile(const char *filename);

/**
 * @brief Format SPIFFS filesystem (WARNING: deletes all files!)
 */
void SPIFFS_formatFS();

/**
 * @brief Check SPIFFS usage stats
 */
void SPIFFS_printStats();

#endif // SPIFFS_LOGGER_H
