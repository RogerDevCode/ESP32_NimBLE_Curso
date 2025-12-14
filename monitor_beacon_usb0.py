#!/usr/bin/env python3
"""
Script de monitoreo para el Beacon (ESP32-WROOM) en /dev/ttyUSB0
"""
import serial
import time
import sys

def monitor_beacon():
    port = '/dev/ttyUSB0'
    baud = 115200
    
    print(f"--- Monitoreando Beacon en {port} ---")
    
    try:
        ser = serial.Serial(port, baud, timeout=1)
        # Reset simple via DTR/RTS
        ser.dtr = False
        ser.rts = True
        time.sleep(0.1)
        ser.rts = False
        time.sleep(1)
        
        start_time = time.time()
        while (time.time() - start_time) < 15: # Monitorear por 15 segundos
            if ser.in_waiting:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
                if line:
                    print(f"[BEACON] {line}")
            time.sleep(0.01)
            
        ser.close()
        print("\n--- Fin del monitoreo del Beacon ---")
        
    except Exception as e:
        print(f"Error accediendo a {port}: {e}")

if __name__ == "__main__":
    monitor_beacon()
