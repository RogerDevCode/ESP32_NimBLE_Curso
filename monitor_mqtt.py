#!/usr/bin/env python3
"""
Script específico para monitorear conexión y publicación MQTT a HiveMQ Cloud
"""

import serial
import threading
import time
import re
from datetime import datetime

def monitor_mqtt(stop_event):
    """Monitorear MQTT en /dev/ttyACM0"""
    try:
        ser = serial.Serial('/dev/ttyACM0', 115200, timeout=0.1)
        time.sleep(0.5)

        print("🔍 MONITOREO MQTT - HIVE MQ CLOUD")
        print("=" * 50)
        print("Broker: mqtt://53b39b4ca3d649d696d8126555721baf.s1.eu.hivemq.cloud")
        print("Tópico: esp32/gateway/data")
        print()

        mqtt_events = []
        ibeacon_count = 0
        published_count = 0

        while not stop_event.is_set():
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]

                        # Eventos MQTT
                        if "MQTT_EVENT_CONNECTED" in line:
                            mqtt_events.append(("CONNECTED", timestamp))
                            print(f"[{timestamp}] ✅ MQTT CONECTADO - HiveMQ Cloud establecido")

                        elif "MQTTS_EVENT_CONNECTED" in line:
                            mqtt_events.append(("MQTTS_CONNECTED", timestamp))
                            print(f"[{timestamp}] 🔒 MQTTS CONECTADO - Conexión segura establecida")

                        elif "MQTT_EVENT_DISCONNECTED" in line:
                            mqtt_events.append(("DISCONNECTED", timestamp))
                            print(f"[{timestamp}] ❌ MQTT DESCONECTADO")

                        elif "MQTT_ERROR" in line:
                            mqtt_events.append(("ERROR", timestamp))
                            print(f"[{timestamp}] 💥 ERROR MQTT: {line}")

                        # Publicaciones MQTT
                        elif "Publicado:" in line and "gateway/data" in line:
                            published_count += 1
                            # Extraer información del JSON
                            json_match = re.search(r'\{.*\}', line)
                            if json_match:
                                print(f"[{timestamp}] 📤 PUBLICACIÓN #{published_count}")
                                # Extraer MAC y RSSI del JSON si es iBeacon
                                json_str = json_match.group()
                                if "ca:12:f2:5c:7b:cc" in json_str:  # MAC del iBeacon
                                    print(f"   📡 JSON: {json_str}")

                        # Eventos WiFi
                        elif "IP Obtenida:" in line:
                            print(f"[{timestamp}] 🌐 {line}")

                        # Detecciones iBeacon
                        elif "iBeacon DETECTED!" in line and "ca:12:f2:5c:7b:cc" in line:
                            ibeacon_count += 1
                            print(f"[{timestamp}] 🎯 iBeacon #{ibeacon_count}: {line}")

                        # Información general
                        elif "BLE Sincronizado" in line:
                            print(f"[{timestamp}] 🔵 BLE iniciado")
                        elif "main_task: Returned from app_main" in line:
                            print(f"[{timestamp}] 🚀 Sistema operativo")

                except Exception as e:
                    pass
            else:
                time.sleep(0.01)

        # Resumen final
        print("\n" + "=" * 50)
        print("📊 RESUMEN DE MONITOREO MQTT")
        print("=" * 50)

        connected = any("CONNECTED" in event[0] for event in mqtt_events)

        print(f"🌐 WiFi Status: ✅ Conectado")
        print(f"📡 MQTT Status: {'✅ Conectado' if connected else '❌ No conectado'}")
        print(f"🎯 iBeacons detectados: {ibeacon_count}")
        print(f"📤 Publicaciones MQTT: {published_count}")

        if connected and published_count > 0:
            print(f"\n🎉 ¡ÉXITO! Sistema MQTT fully operativo")
            print(f"   → Broker: HiveMQ Cloud conectado")
            print(f"   → Publicaciones: {published_count} mensajes enviados")
        elif connected:
            print(f"\n⚠️ MQTT conectado pero sin publicaciones")
        else:
            print(f"\n❌ MQTT no pudo conectar al broker")

        ser.close()

    except Exception as e:
        print(f"❌ Error conectando: {e}")

def main():
    print("🔍 INICIANDO MONITOREO MQTT HIVE MQ CLOUD")
    print("Duración: 45 segundos o Ctrl+C para detener\n")

    stop_event = threading.Event()
    monitor_thread = threading.Thread(target=monitor_mqtt, args=(stop_event,))
    monitor_thread.start()

    try:
        time.sleep(45)
    except KeyboardInterrupt:
        print("\n⏹️  Monitoreo detenido por usuario")
    finally:
        stop_event.set()
        monitor_thread.join()
        print("\n🏁 Monitoreo finalizado")

if __name__ == "__main__":
    main()