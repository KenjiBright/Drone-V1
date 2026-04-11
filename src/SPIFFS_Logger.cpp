#include "SPIFFS_Logger.h"
#include <time.h>

// ===== GLOBAL LOGGER STATE =====
SPIFFSLogger_t spiffs_logger = {
    .is_logging = false,
    .bytes_written = 0
};

static uint32_t log_start_time_ms = 0;
static char csv_buffer[CSV_BUFFER_SIZE];
static uint32_t log_file_counter = 0;  // Counter for unique filenames

// ===== INTERNAL HELPERS =====

/**
 * Generate filename using millis() uptime counter
 * Format: XXXXXX_XXXXXX_description.csv
 * Where X are digits from uptime millis and counter
 */
static void _make_filename(char *buf, size_t buf_size, const char *description) {
    uint32_t ms = millis();
    uint32_t uptime_sec = ms / 1000;
    
    // Increment counter for each new log file
    log_file_counter++;
    
    // SPIFFS is flat filesystem — no subdirectories
    // Format: /bb_XXXXXX_XXXXXX_description.csv
    snprintf(buf, buf_size, "/bb_%06lu_%06lu_%s.csv",
        uptime_sec % 1000000,
        log_file_counter % 1000000,
        description
    );
}

// ===== PUBLIC API =====

void SPIFFS_init() {
    if (!SPIFFS.begin(true)) {  // true = format if mount fails
        Serial.println("[SPIFFS] Mount FAILED!");
        return;
    }
    
    Serial.println("[SPIFFS] ✓ Initialized");
    SPIFFS_printStats();
}

void SPIFFS_startLog(const char *description) {
    if (spiffs_logger.is_logging) {
        SPIFFS_stopLog();  // Stop previous session
    }
    
    _make_filename(spiffs_logger.filename, sizeof(spiffs_logger.filename), description);
    
    spiffs_logger.current_file = SPIFFS.open(spiffs_logger.filename, "w");
    
    if (!spiffs_logger.current_file) {
        Serial.printf("[SPIFFS] ✗ Failed to create %s\n", spiffs_logger.filename);
        return;
    }
    
    // Write CSV header
    const char *header = "time_ms,thr,"
        "roll_sp,roll_gyro,roll_P,roll_I,roll_D,"
        "pitch_sp,pitch_gyro,pitch_P,pitch_I,pitch_D,"
        "yaw_sp,yaw_gyro,yaw_P,yaw_I,yaw_D\n";
    
    spiffs_logger.current_file.print(header);
    
    spiffs_logger.is_logging = true;
    spiffs_logger.bytes_written = 0;
    log_start_time_ms = millis();
    
    Serial.printf("[SPIFFS] ✓ Started logging to %s\n", spiffs_logger.filename);
}

void SPIFFS_stopLog() {
    if (!spiffs_logger.is_logging || !spiffs_logger.current_file) {
        return;
    }
    
    spiffs_logger.current_file.close();
    spiffs_logger.is_logging = false;
    
    uint32_t elapsed = millis() - log_start_time_ms;
    Serial.printf("[SPIFFS] ✓ Stopped logging (%u ms, %u bytes)\n", 
        elapsed, spiffs_logger.bytes_written);
}

void SPIFFS_log_data(
    int   throttle,
    float roll_sp,  float roll_gyro,  float roll_P,  float roll_I,  float roll_D,
    float pitch_sp, float pitch_gyro, float pitch_P, float pitch_I, float pitch_D,
    float yaw_sp,   float yaw_gyro,   float yaw_P,   float yaw_I,   float yaw_D
) {
    if (!spiffs_logger.is_logging || !spiffs_logger.current_file) {
        return;
    }
    
    uint32_t time_ms = millis() - log_start_time_ms;
    
    // Build CSV row (careful with precision to keep size small)
    snprintf(csv_buffer, CSV_BUFFER_SIZE,
        "%u,%d,"
        "%.2f,%.2f,%.3f,%.3f,%.3f,"
        "%.2f,%.2f,%.3f,%.3f,%.3f,"
        "%.2f,%.2f,%.3f,%.3f,%.3f\n",
        time_ms, throttle,
        roll_sp, roll_gyro, roll_P, roll_I, roll_D,
        pitch_sp, pitch_gyro, pitch_P, pitch_I, pitch_D,
        yaw_sp, yaw_gyro, yaw_P, yaw_I, yaw_D
    );
    
    size_t written = spiffs_logger.current_file.print(csv_buffer);
    spiffs_logger.bytes_written += written;
}

void SPIFFS_exportCSV(const char *filename) {
    char filepath[128];
    snprintf(filepath, sizeof(filepath), "/%s", filename);
    
    if (!SPIFFS.exists(filepath)) {
        Serial.printf("[SPIFFS] ✗ File not found: %s\n", filepath);
        return;
    }
    
    File f = SPIFFS.open(filepath, "r");
    if (!f) {
        Serial.printf("[SPIFFS] ✗ Cannot open file: %s\n", filepath);
        return;
    }
    
    Serial.printf("[SPIFFS] Exporting %s (%u bytes):\n", filename, f.size());
    Serial.println("---BEGIN CSV---");
    
    while (f.available()) {
        size_t chunk_size = f.readBytes(csv_buffer, CSV_BUFFER_SIZE - 1);
        csv_buffer[chunk_size] = '\0';
        Serial.print(csv_buffer);
    }
    
    Serial.println("\n---END CSV---");
    f.close();
    
    Serial.printf("[SPIFFS] ✓ Export complete\n");
}

void SPIFFS_listFiles() {
    File dir = SPIFFS.open("/");
    File file = dir.openNextFile();
    
    Serial.println("[SPIFFS] Log files:");
    int count = 0;
    while (file) {
        Serial.printf("  %3d. %s (%u bytes)\n", 
            ++count, file.name(), file.size());
        file = dir.openNextFile();
    }
    
    if (count == 0) {
        Serial.println("  (empty)");
    }
}

bool SPIFFS_deleteFile(const char *filename) {
    char filepath[128];
    snprintf(filepath, sizeof(filepath), "/%s", filename);
    
    if (!SPIFFS.exists(filepath)) {
        Serial.printf("[SPIFFS] ✗ File not found: %s\n", filepath);
        return false;
    }
    
    if (SPIFFS.remove(filepath)) {
        Serial.printf("[SPIFFS] ✓ Deleted %s\n", filepath);
        return true;
    } else {
        Serial.printf("[SPIFFS] ✗ Failed to delete %s\n", filepath);
        return false;
    }
}

void SPIFFS_formatFS() {
    Serial.println("[SPIFFS] ⚠ WARNING: Formatting filesystem (erasing all files)...");
    if (SPIFFS.format()) {
        Serial.println("[SPIFFS] ✓ Formatted successfully");
        SPIFFS_init();
    } else {
        Serial.println("[SPIFFS] ✗ Format failed");
    }
}

void SPIFFS_printStats() {
    size_t total = SPIFFS.totalBytes();
    size_t used = SPIFFS.usedBytes();
    
    Serial.printf("[SPIFFS] Capacity: %u KB, Used: %u KB (%.1f%%)\n",
        total / 1024, used / 1024, (100.0f * used) / total);
}
