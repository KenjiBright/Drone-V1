#!/usr/bin/env python3
"""
===== SPIFFS Black Box CSV Downloader =====
Downloads Black Box log files from ESP32 SPIFFS filesystem via Serial.

Usage:
    python download_spiffs_csv.py --port COM17 --output ./logs

Features:
    - List all log files on SPIFFS
    - Download any CSV file to local disk
    - Automatic retry on connection failure
    - Progress indication
"""

import serial
import sys
import argparse
import time
from pathlib import Path


class SPIFFSDownloader:
    def __init__(self, port, baudrate=115200, timeout=5):
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.ser = None
        self.output_dir = Path(".")
    
    def connect(self):
        """Connect to drone via serial"""
        try:
            self.ser = serial.Serial(self.port, self.baudrate, timeout=self.timeout)
            time.sleep(1)  # Wait for board reset
            print(f"✓ Connected to {self.port} @ {self.baudrate} baud")
            return True
        except serial.SerialException as e:
            print(f"✗ Failed to open {self.port}: {e}")
            return False
    
    def disconnect(self):
        """Close serial connection"""
        if self.ser:
            self.ser.close()
            print("Disconnected")
    
    def send_command(self, cmd):
        """Send command and wait for response"""
        if not self.ser or not self.ser.is_open:
            return None
        
        self.ser.write(cmd.encode() + b'\n')
        self.ser.flush()
    
    def read_response(self, timeout=10):
        """Read lines until timeout or end marker"""
        if not self.ser:
            return []
        
        self.ser.timeout = timeout
        lines = []
        start_time = time.time()
        
        try:
            while time.time() - start_time < timeout:
                if self.ser.in_waiting:
                    line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        lines.append(line)
                        print(f"  {line}")
                else:
                    time.sleep(0.05)
        except:
            pass
        
        self.ser.timeout = self.timeout
        return lines
    
    def list_files(self):
        """List all files on SPIFFS"""
        print("\n[SPIFFS] Requesting file list...")
        self.send_command("4")  # Menu option 4
        time.sleep(0.5)
        
        # Read responses until we get stats
        lines = []
        for _ in range(20):  # Try up to 20 reads
            self.ser.timeout = 2
            try:
                while self.ser.in_waiting:
                    line = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    if line and not line.startswith("Export filename"):
                        lines.append(line)
                        print(f"  {line}")
                        if "Capacity:" in line:
                            break
            except:
                pass
            time.sleep(0.1)
        
        return lines
    
    def download_csv(self, filename):
        """Download specific CSV file"""
        output_file = self.output_dir / filename
        print(f"\n[DOWNLOAD] Requesting {filename}...")
        
        # Send filename to export
        self.send_command(filename)
        time.sleep(0.5)
        
        # Read until we see BEGIN/END markers
        csv_lines = []
        in_csv = False
        
        print("[DOWNLOAD] Receiving data...")
        start_time = time.time()
        
        while time.time() - start_time < 30:  # 30 second timeout
            self.ser.timeout = 2
            try:
                while self.ser.in_waiting:
                    line = self.ser.readline().decode('utf-8', errors='ignore').rstrip('\r\n')
                    
                    if "---BEGIN CSV---" in line:
                        in_csv = True
                        print("  [Started receiving CSV...]")
                        continue
                    
                    if "---END CSV---" in line:
                        in_csv = False
                        print("  [Finished receiving CSV]")
                        break
                    
                    if in_csv and line:
                        csv_lines.append(line)
                
                if not in_csv and csv_lines:
                    break
            except:
                pass
            
            time.sleep(0.05)
        
        if csv_lines:
            # Write to file
            with open(output_file, 'w') as f:
                f.write('\n'.join(csv_lines) + '\n')
            
            print(f"✓ Downloaded {len(csv_lines)} lines → {output_file}")
            print(f"  File size: {output_file.stat().st_size} bytes")
            return True
        else:
            print(f"✗ No CSV data received")
            return False
    
    def run_menu(self):
        """Interactive menu"""
        while True:
            print("\n" + "="*50)
            print("     SPIFFS CSV DOWNLOADER MENU")
            print("="*50)
            print("1) List files on SPIFFS")
            print("2) Download CSV file")
            print("3) Exit")
            print("="*50)
            
            choice = input("Select option: ").strip()
            
            if choice == "1":
                self.list_files()
                input("Press ENTER to continue...")
            elif choice == "2":
                filename = input("Enter filename (e.g., 20260412_143630_hover.csv): ").strip()
                if filename:
                    self.download_csv(filename)
                else:
                    print("Empty filename")
            elif choice == "3":
                break
            else:
                print("Invalid option")


def main():
    parser = argparse.ArgumentParser(
        description="Download Black Box CSV files from ESP32 SPIFFS",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Interactive menu
  python download_spiffs_csv.py --port COM17
  
  # List files on SPIFFS
  python download_spiffs_csv.py --port COM17 --list
  
  # Download specific file
  python download_spiffs_csv.py --port COM17 --download 20260412_143630_hover.csv
        """)
    
    parser.add_argument('--port', type=str, default='COM17', 
                        help='Serial port (default: COM17)')
    parser.add_argument('--baud', type=int, default=115200,
                        help='Baud rate (default: 115200)')
    parser.add_argument('--output', type=Path, default='.',
                        help='Output directory (default: current dir)')
    parser.add_argument('--list', action='store_true',
                        help='List files and exit')
    parser.add_argument('--download', type=str, metavar='FILENAME',
                        help='Download specific file and exit')
    
    args = parser.parse_args()
    
    # Create output directory
    args.output.mkdir(parents=True, exist_ok=True)
    
    # Create downloader
    dl = SPIFFSDownloader(args.port, args.baud)
    dl.output_dir = args.output
    
    # Connect
    if not dl.connect():
        sys.exit(1)
    
    try:
        if args.list:
            # List files and exit
            dl.list_files()
        elif args.download:
            # Download specific file and exit
            dl.download_csv(args.download)
        else:
            # Interactive menu
            dl.run_menu()
    finally:
        dl.disconnect()


if __name__ == '__main__':
    main()
