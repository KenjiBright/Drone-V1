#!/bin/bash
# Drone V1 - Quick capture script for Linux/macOS

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check Python
if ! command -v python3 &> /dev/null; then
    echo -e "${RED}[ERROR]${NC} Python 3 not found"
    echo "Install: https://www.python.org/downloads/"
    exit 1
fi

# Check requirements
echo "Checking dependencies..."
python3 -m pip show pyserial >/dev/null 2>&1 || {
    echo -e "${YELLOW}[INFO]${NC} Installing dependencies..."
    python3 -m pip install -r requirements.txt
}

# Get parameters
PORT="${1:-/dev/ttyUSB0}"
DURATION="${2:-30}"
DESC="${3:-flight}"

# Display info
echo "============================================================"
echo "          DRONE V1 - BLACK BOX CAPTURE"
echo "============================================================"
echo ""
echo "Port: $PORT"
echo "Time: ${DURATION}s"
echo "Description: $DESC"
echo ""
echo "Press Enter to start..."
read

# Run script
python3 capture_blackbox.py --port "$PORT" --duration "$DURATION" --desc "$DESC"

if [ $? -ne 0 ]; then
    echo -e "${RED}[ERROR]${NC} Something went wrong"
    exit 1
fi
