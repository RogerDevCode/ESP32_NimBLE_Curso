#!/usr/bin/env python3
import serial
import time
import sys

try:
    # Open serial port
    ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)
    print("Serial port opened. Resetting ESP32...")
    
    # Reset ESP32 by toggling DTR
    ser.dtr = False
    time.sleep(0.1)
    ser.dtr = True
    time.sleep(0.5)
    
    print("Capturing output for 15 seconds...")
    start_time = time.time()
    output = []
    
    while time.time() - start_time < 15:
        if ser.in_waiting > 0:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line:
                print(line)
                output.append(line)
        time.sleep(0.1)
    
    ser.close()
    print("\nCapture complete.")
    
    # Save output to file
    with open('output.txt', 'w') as f:
        f.write('\n'.join(output))
    
except serial.SerialException as e:
    print(f"Error opening serial port: {e}")
    sys.exit(1)
except KeyboardInterrupt:
    print("\nCapture interrupted by user.")
    ser.close()