#!/usr/bin/env python3
"""
Drone V1 - Black Box CSV Capture & Analysis Tool
Capture dữ liệu từ Serial port và phân tích PID
"""

import serial
import time
import os
import sys
from datetime import datetime
import argparse

try:
    import pandas as pd
    import matplotlib.pyplot as plt
    PANDAS_AVAILABLE = True
except ImportError:
    PANDAS_AVAILABLE = False
    print("[WARNING] pandas/matplotlib không cài đặt. Chỉ capture CSV, không vẽ biểu đồ.")
    print("         Cài đặt: pip install pandas matplotlib")


class DroneBlackBox:
    """Capture và xử lý Black Box data từ Drone V1"""
    
    def __init__(self, port='COM17', baudrate=115200, timeout=30):
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.ser = None
        self.filename = None
        self.lines_captured = 0
        
    def connect(self):
        """Kết nối serial port"""
        try:
            self.ser = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=1
            )
            print(f"✓ Kết nối {self.port} @ {self.baudrate} baud")
            time.sleep(1)
            return True
        except serial.SerialException as e:
            print(f"✗ Lỗi kết nối: {e}")
            return False
    
    def disconnect(self):
        """Ngắt kết nối"""
        if self.ser and self.ser.is_open:
            self.ser.close()
            print("✓ Ngắt kết nối")
    
    def capture_data(self, duration=30, description="flight"):
        """Capture dữ liệu trong khoảng thời gian"""
        if not self.ser or not self.ser.is_open:
            print("✗ Serial port chưa kết nối")
            return False
        
        # Tạo tên file
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        self.filename = f"flight_{timestamp}_{description}.csv"
        
        print(f"\n{'='*60}")
        print(f"📊 CAPTURE BLACK BOX DATA")
        print(f"{'='*60}")
        print(f"Thời gian: {duration} giây")
        print(f"File: {self.filename}")
        print(f"\n⏳ Hãy nhấn phím 1 trên menu drone để bắt đầu log...")
        print(f"   (Đợi cho đến khi thấy BLACKBOX header)")
        print(f"\nBắt đầu capture trong 5 giây...")
        time.sleep(5)
        
        start_time = time.time()
        header_found = False
        
        try:
            with open(self.filename, 'w') as f:
                while time.time() - start_time < duration:
                    try:
                        line = self.ser.readline().decode('utf-8', errors='ignore')
                        if line.strip():
                            # Lọc dòng CSV (bỏ dòng chứa [BLACKBOX] hoặc khoảng trắng)
                            if line.startswith('time_ms'):
                                header_found = True
                                print(f"\n✓ Tìm thấy CSV header")
                            
                            if header_found:
                                if ',' in line and (line[0].isdigit() or line.startswith('time_ms')):
                                    f.write(line)
                                    f.flush()
                                    self.lines_captured += 1
                                    
                                    # Progress bar
                                    elapsed = time.time() - start_time
                                    pct = int((elapsed / duration) * 100)
                                    bar_len = 40
                                    filled = int(bar_len * elapsed / duration)
                                    bar = '█' * filled + '░' * (bar_len - filled)
                                    print(f"\r[{bar}] {pct:3d}% ({self.lines_captured} rows)", end='', flush=True)
                    except Exception as e:
                        continue
            
            print(f"\n\n✓ Capture hoàn tất!")
            print(f"  Dòng: {self.lines_captured}")
            print(f"  File: {self.filename}")
            return True
            
        except KeyboardInterrupt:
            print("\n\n⚠ Dừng bởi người dùng")
            return False
        except Exception as e:
            print(f"\n✗ Lỗi: {e}")
            return False
    
    def analyze_data(self, show_plot=True):
        """Phân tích dữ liệu CSV"""
        if not self.filename or not os.path.exists(self.filename):
            print("✗ File CSV không tồn tại")
            return False
        
        if not PANDAS_AVAILABLE:
            print("⚠ pandas/matplotlib chưa cài đặt, không thể vẽ biểu đồ")
            print("  Cài đặt: pip install pandas matplotlib")
            return False
        
        print(f"\n{'='*60}")
        print(f"📈 PHÂN TÍCH DỮ LIỆU")
        print(f"{'='*60}")
        
        try:
            # Đọc CSV
            df = pd.read_csv(self.filename)
            print(f"\n✓ Đọc {len(df)} dòng dữ liệu")
            print(f"  Cột: {', '.join(df.columns.tolist())}")
            
            # Thống kê cơ bản
            print(f"\n📊 THỐNG KÊ:")
            print(f"  Thời gian bay: {df['time_ms'].max() / 1000:.1f}s")
            print(f"  Throttle trung bình: {df['thr'].mean():.0f} µs")
            print(f"  Throttle max: {df['thr'].max():.0f} µs")
            
            # Tính độ lệch (error) cho Roll, Pitch, Yaw
            df['roll_error'] = df['roll_sp'] - df['roll_gyro']
            df['pitch_error'] = df['pitch_sp'] - df['pitch_gyro']
            df['yaw_error'] = df['yaw_sp'] - df['yaw_gyro']
            
            print(f"\n🎛️ PID PERFORMANCE:")
            print(f"\n  Roll:")
            print(f"    Error max: {df['roll_error'].abs().max():.2f}°")
            print(f"    Error RMS: {(df['roll_error']**2).mean()**0.5:.2f}°")
            print(f"  Pitch:")
            print(f"    Error max: {df['pitch_error'].abs().max():.2f}°")
            print(f"    Error RMS: {(df['pitch_error']**2).mean()**0.5:.2f}°")
            print(f"  Yaw:")
            print(f"    Error max: {df['yaw_error'].abs().max():.2f}°")
            print(f"    Error RMS: {(df['yaw_error']**2).mean()**0.5:.2f}°")
            
            # Vẽ biểu đồ
            if show_plot:
                print(f"\n📊 Vẽ biểu đồ...")
                self._plot_analysis(df)
                
            return True
            
        except Exception as e:
            print(f"✗ Lỗi phân tích: {e}")
            return False
    
    def _plot_analysis(self, df):
        """Vẽ biểu đồ phân tích PID"""
        fig, axes = plt.subplots(3, 2, figsize=(15, 10))
        fig.suptitle(f'Drone V1 Flight Analysis - {os.path.basename(self.filename)}', fontsize=16)
        
        # ===== Roll =====
        axes[0, 0].plot(df['time_ms'] / 1000, df['roll_sp'], label='Setpoint', color='blue', linewidth=2)
        axes[0, 0].plot(df['time_ms'] / 1000, df['roll_gyro'], label='Actual', color='red', alpha=0.7)
        axes[0, 0].set_ylabel('Roll (°/s)')
        axes[0, 0].set_title('Roll Response')
        axes[0, 0].legend()
        axes[0, 0].grid(alpha=0.3)
        
        axes[0, 1].plot(df['time_ms'] / 1000, df['roll_P'], label='P', linewidth=1)
        axes[0, 1].plot(df['time_ms'] / 1000, df['roll_I'], label='I', linewidth=1)
        axes[0, 1].plot(df['time_ms'] / 1000, df['roll_D'], label='D', linewidth=1)
        axes[0, 1].set_ylabel('PID Output')
        axes[0, 1].set_title('Roll PID Components')
        axes[0, 1].legend()
        axes[0, 1].grid(alpha=0.3)
        
        # ===== Pitch =====
        axes[1, 0].plot(df['time_ms'] / 1000, df['pitch_sp'], label='Setpoint', color='blue', linewidth=2)
        axes[1, 0].plot(df['time_ms'] / 1000, df['pitch_gyro'], label='Actual', color='red', alpha=0.7)
        axes[1, 0].set_ylabel('Pitch (°/s)')
        axes[1, 0].set_title('Pitch Response')
        axes[1, 0].legend()
        axes[1, 0].grid(alpha=0.3)
        
        axes[1, 1].plot(df['time_ms'] / 1000, df['pitch_P'], label='P', linewidth=1)
        axes[1, 1].plot(df['time_ms'] / 1000, df['pitch_I'], label='I', linewidth=1)
        axes[1, 1].plot(df['time_ms'] / 1000, df['pitch_D'], label='D', linewidth=1)
        axes[1, 1].set_ylabel('PID Output')
        axes[1, 1].set_title('Pitch PID Components')
        axes[1, 1].legend()
        axes[1, 1].grid(alpha=0.3)
        
        # ===== Yaw =====
        axes[2, 0].plot(df['time_ms'] / 1000, df['yaw_sp'], label='Setpoint', color='blue', linewidth=2)
        axes[2, 0].plot(df['time_ms'] / 1000, df['yaw_gyro'], label='Actual', color='red', alpha=0.7)
        axes[2, 0].set_ylabel('Yaw (°/s)')
        axes[2, 0].set_xlabel('Time (s)')
        axes[2, 0].set_title('Yaw Response')
        axes[2, 0].legend()
        axes[2, 0].grid(alpha=0.3)
        
        axes[2, 1].plot(df['time_ms'] / 1000, df['yaw_P'], label='P', linewidth=1)
        axes[2, 1].plot(df['time_ms'] / 1000, df['yaw_I'], label='I', linewidth=1)
        axes[2, 1].plot(df['time_ms'] / 1000, df['yaw_D'], label='D', linewidth=1)
        axes[2, 1].set_ylabel('PID Output')
        axes[2, 1].set_xlabel('Time (s)')
        axes[2, 1].set_title('Yaw PID Components')
        axes[2, 1].legend()
        axes[2, 1].grid(alpha=0.3)
        
        plt.tight_layout()
        
        # Lưu biểu đồ
        plot_file = self.filename.replace('.csv', '.png')
        plt.savefig(plot_file, dpi=150)
        print(f"✓ Biểu đồ lưu: {plot_file}")
        
        # Hiển thị
        plt.show()


def main():
    """Main entry point"""
    parser = argparse.ArgumentParser(
        description='Drone V1 Black Box Capture & Analysis',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Ví dụ:
  python capture_blackbox.py --port COM17 --duration 30 --desc "test1"
  python capture_blackbox.py --analyze flight_20260412_143630_test1.csv
        """
    )
    parser.add_argument('--port', default='COM17', help='Serial port (default: COM17)')
    parser.add_argument('--baudrate', type=int, default=115200, help='Baud rate (default: 115200)')
    parser.add_argument('--duration', type=int, default=30, help='Capture duration in seconds (default: 30)')
    parser.add_argument('--desc', default='flight', help='Flight description (default: flight)')
    parser.add_argument('--analyze', help='Analyze existing CSV file')
    parser.add_argument('--no-plot', action='store_true', help='Skip plotting')
    
    args = parser.parse_args()
    
    # Nếu chỉ phân tích file cũ
    if args.analyze:
        if not os.path.exists(args.analyze):
            print(f"✗ File không tồn tại: {args.analyze}")
            sys.exit(1)
        
        blackbox = DroneBlackBox()
        blackbox.filename = args.analyze
        blackbox.analyze_data(show_plot=not args.no_plot)
        return
    
    # Capture mới
    print(f"\n{'='*60}")
    print(f"🚁 DRONE V1 - BLACK BOX CAPTURE TOOL")
    print(f"{'='*60}")
    
    blackbox = DroneBlackBox(port=args.port, baudrate=args.baudrate)
    
    # Kết nối
    if not blackbox.connect():
        sys.exit(1)
    
    try:
        # Capture dữ liệu
        if not blackbox.capture_data(duration=args.duration, description=args.desc):
            sys.exit(1)
        
        # Phân tích ngay sau capture
        time.sleep(1)
        blackbox.analyze_data(show_plot=not args.no_plot)
        
    finally:
        blackbox.disconnect()
    
    print(f"\n{'='*60}")
    print(f"✓ Xong!")
    print(f"{'='*60}\n")


if __name__ == '__main__':
    main()
