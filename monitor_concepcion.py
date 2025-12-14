#!/usr/bin/env python3
"""
Script para monitorear mensajes MQTT del Gateway a /concepcion/esp32/contador y /concepcion/esp32/ibeacon
"""

import serial
import threading
import time
import re
import json
from datetime import datetime

def monitor_gateway_concepcion(stop_event):
    """Monitorear el gateway ESP32-S3 para tópicos /concepcion/esp32/*"""
    try:
        ser = serial.Serial('/dev/ttyACM0', 115200, timeout=0.1)
        time.sleep(0.5)

        print("🚀 MONITOREO GATEWAY CONCEPCIÓN")
        print("=" * 50)
        print("Dispositivo: ESP32-S3 Gateway")
        print("Tópicos: /concepcion/esp32/contador y /concepcion/esp32/ibeacon")
        print("Esperando inicialización...\n")

        contador_recibidos = 0
        ibeacon_recibidos = 0
        mqtt_conectado = False
        wifi_conectado = False

        while not stop_event.is_set():
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]

                        # Eventos de conexión
                        if "MQTT_EVENT_CONNECTED" in line:
                            mqtt_conectado = True
                            print(f"[{timestamp}] 🔗 MQTT CONECTADO - HiveMQ Cloud TLS")

                        elif "IP Obtenida:" in line:
                            wifi_conectado = True
                            print(f"[{timestamp}] 🌐 WiFi conectado")

                        # Eventos de Heartbeat Counter
                        elif "Heartbeat #" in line and "enviado a" in line:
                            contador_recibidos += 1
                            # Extraer número del heartbeat
                            match = re.search(r'Heartbeat #(\d+) enviado', line)
                            if match:
                                num = match.group(1)
                                print(f"[{timestamp}] ❤️  CONTADOR #{num} - Heartbeat enviado")

                        # Eventos de iBeacon
                        elif "Publicado:" in line:
                            # Intentar parsear JSON para ver si es iBeacon
                            try:
                                if '{' in line:
                                    json_start = line.find('{')
                                    json_str = line[json_start:]
                                    data = json.loads(json_str)

                                    if 'uuid' in data and 'major' in data and 'minor' in data:
                                        # Es un iBeacon
                                        ibeacon_recibidos += 1
                                        mac = data.get('addr', 'unknown')
                                        rssi = data.get('rssi', 0)
                                        uuid = data.get('uuid', '')[:8] + '...'
                                        print(f"[{timestamp}] 🎯 iBEACON #{ibeacon_recibidos}")
                                        print(f"    MAC: {mac} | RSSI: {rssi} | UUID: {uuid}")
                                        print(f"    Major: {data.get('major', 'N/A')} | Minor: {data.get('minor', 'N/A')}")

                            except:
                                # Si no es JSON, solo mostrar la línea
                                if "addr" in line and "rssi" in line:
                                    ibeacon_recibidos += 1
                                    print(f"[{timestamp}] 🎯 iBeacon detectado: {line}")

                        # Eventos informativos
                        elif "Heartbeat Counter Task iniciada" in line:
                            print(f"[{timestamp}] ⏰ Tarea Heartbeat iniciada")

                        elif "BLE Sincronizado" in line:
                            print(f"[{timestamp}] 🔵 BLE escaneo iniciado")

                        # Errores
                        elif "Error" in line or "FAIL" in line:
                            print(f"[{timestamp}] ❌ {line}")

                except Exception as e:
                    pass
            else:
                time.sleep(0.01)

        # Resumen final
        print("\n" + "=" * 50)
        print("📊 RESUMEN DE MONITOREO")
        print("=" * 50)
        print(f"🌐 WiFi Status: {'✅ Conectado' if wifi_conectado else '❌ No conectado'}")
        print(f"📡 MQTT Status: {'✅ Conectado' if mqtt_conectado else '❌ No conectado'}")
        print(f"❤️  Heartbeat enviados: {contador_recibidos}")
        print(f"🎯 iBeacons detectados: {ibeacon_recibidos}")

        if mqtt_conectado:
            print(f"\n📈 ESTADÍSTICAS DE ENVÍO:")
            print(f"   → Tópico /concepcion/esp32/contador: {contador_recibidos} mensajes")
            print(f"   → Tópico /concepcion/esp32/ibeacon: {ibeacon_recibidos} mensajes")
            print(f"   → Total MQTT messages: {contador_recibidos + ibeacon_recibidos}")
            print(f"\n🎉 Gateway CONCEPCIÓN fully operativo!")

        ser.close()

    except Exception as e:
        print(f"❌ Error: {e}")

def main():
    print("🚀 INICIANDO MONITOREO CONCEPCIÓN ESP32")
    print("Duración: 60 segundos o Ctrl+C para detener\n")

    stop_event = threading.Event()
    monitor_thread = threading.Thread(target=monitor_gateway_concepcion, args=(stop_event,))
    monitor_thread.start()

    try:
        time.sleep(60)
    except KeyboardInterrupt:
        print("\n⏹️  Monitoreo detenido por usuario")
    finally:
        stop_event.set()
        monitor_thread.join()
        print("\n🏁 Monitoreo finalizado")

if __name__ == "__main__":
    main()