@echo off
REM Drone V1 - Quick capture script for Windows

setlocal enabledelayedexpansion

REM Tìm Python
where python >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Python khong co tren PATH
    echo Cai dat Python: https://www.python.org/downloads/
    pause
    exit /b 1
)

REM Kiem tra requirements
echo Kiem tra dependencies...
python -m pip show pyserial >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [INFO] Cai dat dependencies...
    python -m pip install -r requirements.txt
    if %ERRORLEVEL% NEQ 0 (
        echo [ERROR] Khong the cai dat dependencies
        pause
        exit /b 1
    )
)

REM Lay thong so
IF "%1"=="" (
    set PORT=COM17
    set DURATION=30
    set DESC=flight
) ELSE (
    set PORT=%1
    if "%2"=="" (set DURATION=30) else (set DURATION=%2)
    if "%3"=="" (set DESC=flight) else (set DESC=%3)
)

echo ============================================================
echo           DRONE V1 - BLACK BOX CAPTURE
echo ============================================================
echo.
echo Port: %PORT%
echo Thoi gian: %DURATION%s
echo Mo ta: %DESC%
echo.
echo Nhan Enter de bat dau...
pause

REM Chay script
python capture_blackbox.py --port %PORT% --duration %DURATION% --desc %DESC%

if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Co loi xay ra
    pause
)

endlocal
