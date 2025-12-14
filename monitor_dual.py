#!/usr/bin/env python3
"""
Script de monitoreo dual para verificar comunicación BLE Server-Client
Monitorea ambos puertos serie simultáneamente para verificar el funcionamiento
del contador que se actualiza cada 1 segundo.
"""

import serial
import threading
import time
from datetime import datetime

def monitor_port(port, name, stop_event):
    """Monitoriza un puerto serie específico"""
    try:
        # DTR=0, RTS=0 para normal
        # Para reset: DTR=0, RTS=1 (Active low reset usually)
        ser = serial.Serial(port, 115200, timeout=0.1)
        
        # Reset Sequence
        ser.dtr = False
        ser.rts = True
        time.sleep(0.1)
        ser.rts = False
        time.sleep(0.1)
        
        print(f"[{name}] Conectado a {port} (Reset enviado)")

        while not stop_event.is_set():
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                        print(f"[{timestamp}] [{name}] {line}")
                except Exception:
                    pass
            else:
                time.sleep(0.01)

        ser.close()

    except Exception as e:
        print(f"[{name}] Error: {e}")

def main():
    print("=== MONITOREO DUAL BLE ESP32 (Auto-Reset) ===")
    print("Servidor: /dev/ttyUSB0")
    print("Cliente: /dev/ttyUSB1")
    print("Capturando logs por 30 segundos...\n")

    stop_event = threading.Event()

    server_thread = threading.Thread(target=monitor_port,
                                    args=("/dev/ttyUSB0", "SERVER", stop_event))
    client_thread = threading.Thread(target=monitor_port,
                                    args=("/dev/ttyUSB1", "CLIENT", stop_event))

    server_thread.start()
    client_thread.start()

    try:
        time.sleep(30)
    except KeyboardInterrupt:
        pass
    finally:
        stop_event.set()
        server_thread.join()
        client_thread.join()
        print("\nMonitoreo finalizado.")

if __name__ == "__main__":
    main()