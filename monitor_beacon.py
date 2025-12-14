#!/usr/bin/env python3
"""
Script de monitoreo dual para Scanner (USB0) y Beacon (USB1)
Mejorado para Proyecto 8: Escáner de Beacons
"""

import serial
import threading
import time
from datetime import datetime

def monitor_port(port, name, stop_event):
    """Monitoriza un puerto serie específico"""
    try:
        # Configuración estándar ESP32
        ser = serial.Serial(port, 115200, timeout=0.1)
        
        # Secuencia de Reset (DTR=0, RTS=1 -> RTS=0)
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
                        # Log simple
                        print(f"[{timestamp}] [{name}] {line}")
                except Exception:
                    pass
            else:
                time.sleep(0.01)

        ser.close()

    except Exception as e:
        print(f"[{name}] Error conectando: {e}")

def main():
    print("=== MONITOREO DE BEACONS (Scanner vs Beacon) ===")
    print("Scanner (Main): /dev/ttyUSB0")
    print("Beacon (Client): /dev/ttyUSB1")
    print("Capturando logs por 60 segundos...\n")

    stop_event = threading.Event()

    # USB0 = Scanner (Proyecto 8)
    scanner_thread = threading.Thread(target=monitor_port,
                                    args=('/dev/ttyUSB0', 'SCANNER', stop_event))
    
    # USB1 = Beacon (Proyecto 2 Modificado)
    beacon_thread = threading.Thread(target=monitor_port,
                                    args=('/dev/ttyUSB1', 'BEACON ', stop_event))

    scanner_thread.start()
    beacon_thread.start()

    try:
        # Tiempo extendido para validar detección
        time.sleep(60)
    except KeyboardInterrupt:
        print("\nInterrupción de usuario detectada.")
    finally:
        stop_event.set()
        scanner_thread.join()
        beacon_thread.join()
        print("\nMonitoreo finalizado.")

if __name__ == "__main__":
    main()
