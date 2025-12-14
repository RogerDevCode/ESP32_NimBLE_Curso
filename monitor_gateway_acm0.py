#!/usr/bin/env python3
"""
Script de monitoreo para el Gateway BLE-MQTT (Proyecto 10)
Adaptado para /dev/ttyACM0 - ESP32-S3
"""

import serial
import threading
import time
import json
from datetime import datetime

def monitor_acm0(stop_event):
    """Monitoriza el puerto ACM0 donde está conectado el ESP32-S3"""
    try:
        # Configuración para ESP32-S3
        ser = serial.Serial('/dev/ttyACM0', 115200, timeout=0.1)

        # Secuencia de Reset
        ser.dtr = False
        ser.rts = True
        time.sleep(0.1)
        ser.rts = False
        time.sleep(0.5)  # Esperar más tiempo para el reset completo

        print(f"[ESP32-S3] Conectado a /dev/ttyACM0 (Gateway BLE-MQTT)")
        print("[ESP32-S3] Esperando inicialización del sistema...")

        line_count = 0
        start_time = time.time()

        while not stop_event.is_set():
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                        line_count += 1

                        # Detectar diferentes tipos de mensajes
                        if "wifi" in line.lower() or "connected" in line.lower():
                            print(f"[{timestamp}] [WIFI] {line}")
                        elif "mqtt" in line.lower() or "broker" in line.lower():
                            print(f"[{timestamp}] [MQTT] {line}")
                        elif "ble" in line.lower() or "ibeacon" in line.lower():
                            print(f"[{timestamp}] [BLE ] {line}")
                        elif "gateway" in line.lower() or "json" in line.lower():
                            print(f"[{timestamp}] [GATEWAY] {line}")
                        elif "error" in line.lower() or "fail" in line.lower():
                            print(f"[{timestamp}] [ERROR] {line}")
                        else:
                            print(f"[{timestamp}] [INFO] {line}")

                except Exception as e:
                    print(f"[ERROR] Decodificando línea: {e}")
            else:
                time.sleep(0.01)

        elapsed = time.time() - start_time
        print(f"\n[ESTADÍSTICA] Monitoreo finalizado. {line_count} líneas capturadas en {elapsed:.1f} segundos")
        ser.close()

    except Exception as e:
        print(f"[ERROR] Conectando a /dev/ttyACM0: {e}")
        print("Asegúrese de que el ESP32-S3 esté conectado y no esté siendo utilizado por otro proceso")

def main():
    print("=== MONITOREO GATEWAY BLE-MQTT (PROYECTO 10) ===")
    print("Dispositivo: ESP32-S3 en /dev/ttyACM0")
    print("Función: Gateway BLE Scanner → MQTT")
    print("Duración: 30 segundos o Ctrl+I para detener\n")

    stop_event = threading.Event()

    # Iniciar monitoreo en hilo separado
    monitor_thread = threading.Thread(target=monitor_acm0, args=(stop_event,))
    monitor_thread.start()

    try:
        # Monitorear por 30 segundos o hasta interrupción
        time.sleep(30)
    except KeyboardInterrupt:
        print("\nInterrupción de usuario detectada.")
    finally:
        stop_event.set()
        monitor_thread.join()
        print("Monitoreo finalizado.")

if __name__ == "__main__":
    main()