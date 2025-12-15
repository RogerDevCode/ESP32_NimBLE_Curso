#!/usr/bin/env python3
import serial
import time
import sys
import signal
import os

# Configuration
PORT = '/dev/ttyACM0'
BAUD = 115200
TIMEOUT = 45 # Monitor for 45 seconds

def signal_handler(sig, frame):
    print("\nMonitor interrupted by user.")
    sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)

def monitor():
    print(f"--- Monitoring {PORT} for {TIMEOUT} seconds ---")
    print("Note: On ESP32-S3 Deep Sleep, USB might disconnect/reconnect.")
    
    start_time = time.time()
    ser = None

    while (time.time() - start_time) < TIMEOUT:
        try:
            if not os.path.exists(PORT):
                if ser:
                    print(f"[{time.strftime('%H:%M:%S')}] Device disconnected (Sleeping?)")
                    ser.close()
                    ser = None
                time.sleep(0.1)
                continue

            if ser is None:
                try:
                    ser = serial.Serial(PORT, BAUD, timeout=0.1)
                    print(f"[{time.strftime('%H:%M:%S')}] Device connected/woke up!")
                    # Don't reset on connect, just listen
                    ser.dtr = False
                    ser.rts = False
                except Exception as e:
                    # Port exists but busy or not ready
                    time.sleep(0.1)
                    continue

            if ser.in_waiting:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        timestamp = time.strftime("%H:%M:%S")
                        print(f"[{timestamp}] {line}")
                except Exception:
                    pass
            else:
                time.sleep(0.01)

        except OSError:
            # Handle sudden disconnection while reading
            if ser:
                print(f"[{time.strftime('%H:%M:%S')}] IO Error (Device went to sleep?)")
                ser.close()
                ser = None
            time.sleep(0.1)
                
    if ser:
        ser.close()
    print("\n--- Monitoring finished ---")

if __name__ == "__main__":
    monitor()