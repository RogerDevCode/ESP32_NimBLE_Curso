#!/usr/bin/env python3
"""
Script de Monitoreo Dual: iBeacon (ESP32-WROOM) + Gateway (ESP32-S3)
"""

import serial
import threading
import time
from datetime import datetime

def monitor_esp32_wroom(stop_event, results):
    """Monitorear iBeacon en /dev/ttyUSB0"""
    try:
        ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=0.1)
        time.sleep(0.5)  # Reset manual

        print("🎯 ESP32-WROOM (iBeacon) - Monitoreo iniciado")

        beacon_detected = False

        while not stop_event.is_set():
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]

                        if "BLE Host sincronizado" in line:
                            print(f"[{timestamp}] 🎯 BLE_HOST sincronizado")
                        elif "iBeacon + Name Broadcasting Started" in line:
                            print(f"[{timestamp}] 📡 ¡IBEACON ACTIVO!")
                            beacon_detected = True
                        elif "app_main finalizado" in line:
                            print(f"[{timestamp}] ✅ Sistema iniciado")
                        elif "Watchdog Task Started" in line:
                            print(f"[{timestamp}] 🛡️ Watchdog activo")
                        elif "BLE Host iniciada" in line:
                            print(f"[{timestamp}] 🔄 Host BLE iniciado")
                        elif "Error" in line or "Failed" in line:
                            print(f"[{timestamp}] ❌ ERROR: {line}")

                        results['beacon_running'] = beacon_detected

                except Exception as e:
                    pass
            else:
                time.sleep(0.01)

        ser.close()
        results['beacon_status'] = 'running' if beacon_detected else 'stopped'

    except Exception as e:
        print(f"❌ Error ESP32-WROOM: {e}")
        results['beacon_status'] = 'error'

def monitor_esp32_s3_gateway(stop_event, results):
    """Monitorear Gateway en /dev/ttyACM0"""
    try:
        ser = serial.Serial('/dev/ttyACM0', 115200, timeout=0.1)
        time.sleep(0.5)  # Reset manual

        print("🌐 ESP32-S3 (Gateway) - Monitoreo iniciado")

        ibeacon_detected = False

        while not stop_event.is_set():
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]

                        if "BLE Sincronizado. Iniciando escaneo..." in line:
                            print(f"[{timestamp}] 🔵 Gateway BLE iniciado")
                        elif "iBeacon DETECTED!" in line:
                            ibeacon_detected = True
                            print(f"[{timestamp}] 🎯 ¡IBEACON DETECTADO!")
                            # Extraer info del iBeacon
                            if "Addr:" in line and "RSSI:" in line:
                                print(f"[{timestamp}] 📡 {line}")
                        elif "Publicado:" in line and "gateway/data" in line:
                            print(f"[{timestamp}] 📤 {line}")
                        elif "IP Obtenida:" in line:
                            print(f"[{timestamp}] 🌐 {line}")
                        elif "WiFi desconectado" in line:
                            print(f"[{timestamp}] ⚠️ {line}")
                        elif "MQTT" in line:
                            print(f"[{timestamp}] 📡 {line}")

                        results['ibeacon_detected'] = ibeacon_detected

                except Exception as e:
                    pass
            else:
                time.sleep(0.01)

        ser.close()
        results['gateway_status'] = 'running'

    except Exception as e:
        print(f"❌ Error ESP32-S3: {e}")
        results['gateway_status'] = 'error'

def main():
    print("🔍 MONITOREO DUAL ESP32")
    print("=" * 50)
    print("ESP32-WROOM (/dev/ttyUSB0) - iBeacon")
    print("ESP32-S3 (/dev/ttyACM0) - Gateway BLE-MQTT")
    print("Duración: 30 segundos o Ctrl+C para detener\n")

    stop_event = threading.Event()
    results = {}

    # Iniciar hilos de monitoreo
    wroom_thread = threading.Thread(target=monitor_esp32_wroom, args=(stop_event, results))
    s3_thread = threading.Thread(target=monitor_esp32_s3_gateway, args=(stop_event, results))

    wroom_thread.start()
    s3_thread.start()

    try:
        time.sleep(30)
    except KeyboardInterrupt:
        print("\n⏹️  Monitoreo detenido por usuario")
    finally:
        stop_event.set()
        wroom_thread.join()
        s3_thread.join()

        # Resumen final
        print("\n" + "=" * 50)
        print("📊 RESUMEN FINAL")
        print("=" * 50)
        print(f"🎯 iBeacon Status: {results.get('beacon_status', 'unknown')}")
        print(f"🌐 Gateway Status: {results.get('gateway_status', 'unknown')}")
        print(f"🔍 iBeacon Detectado: {'✅ SÍ' if results.get('ibeacon_detected') else '❌ NO'}")

        if results.get('beacon_running') and results.get('ibeacon_detected'):
            print("\n🎉 ¡ÉXITO COMPLETO! iBeacon detectado por el Gateway")
        elif results.get('beacon_running'):
            print("\n⚠️ iBeacon activo pero no detectado (distancia o interferencia)")
        else:
            print("\n❌ Revisar conexión del ESP32-WROOM")

        print("\n🏁 Monitoreo finalizado")

if __name__ == "__main__":
    main()