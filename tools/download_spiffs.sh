#!/bin/bash
# ===== SPIFFS CSV DOWNLOADER - Linux/Mac Quick Start =====
# This script downloads Black Box log files from ESP32 SPIFFS

# Check Python
if ! command -v python3 &> /dev/null; then
    echo "ERROR: Python3 not found"
    echo "Install: sudo apt install python3-pip (Ubuntu/Debian) or brew install python3 (macOS)"
    exit 1
fi

# Detect COM port
COM_PORT="${1:-}"
if [ -z "$COM_PORT" ]; then
    # Try common Linux/Mac ports
    for port in /dev/ttyUSB0 /dev/ttyUSB1 /dev/ttyACM0 /dev/ttyACM1 /dev/cu.usbserial* /dev/cu.wchusbserial*; do
        if [ -e "$port" ]; then
            COM_PORT="$port"
            echo "Found ESP32 on: $COM_PORT"
            break
        fi
    done
    
    if [ -z "$COM_PORT" ]; then
        echo "ERROR: No ESP32 found on /dev/ttyUSB* or /dev/ttyACM*"
        echo "Try: ls /dev/tty* | grep -E 'USB|ACM|serial'"
        echo "Then run: ./download_spiffs.sh /dev/ttyUSB0"
        exit 1
    fi
fi

echo "Using port: $COM_PORT"
echo ""

# Run downloader
python3 download_spiffs_csv.py --port "$COM_PORT"

if [ $? -eq 0 ]; then
    echo ""
    echo "Done! Check current directory for CSV files"
else
    echo "ERROR: Download failed"
    exit 1
fi
