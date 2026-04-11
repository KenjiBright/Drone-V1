@echo off
REM ===== SPIFFS CSV DOWNLOADER - Windows Quick Start =====
REM This script downloads Black Box log files from ESP32 SPIFFS

setlocal enabledelayedexpansion

REM Check Python
python --version >nul 2>&1
if errorlevel 1 (
    echo ERROR: Python not found. Install from python.org or Microsoft Store
    pause
    exit /b 1
)

REM Get COM port
if "%1"=="" (
    echo Scanning for ESP32 on COM ports...
    for /l %%i in (1,1,20) do (
        mode COM%%i >nul 2>&1
        if not errorlevel 1 (
            echo Found: COM%%i
            set "COM_PORT=COM%%i"
            goto :found
        )
    )
    if not defined COM_PORT (
        echo ERROR: No COM port found
        echo Please connect ESP32 and try again, or specify: download_spiffs.bat COM17
        pause
        exit /b 1
    )
) else (
    set "COM_PORT=%1"
)

:found
echo.
echo Using port: !COM_PORT!
echo.

REM Run downloader
python download_spiffs_csv.py --port !COM_PORT!

if errorlevel 1 (
    echo ERROR: Download failed
    pause
    exit /b 1
)

echo.
echo Done! Check the local directory for CSV files
pause
