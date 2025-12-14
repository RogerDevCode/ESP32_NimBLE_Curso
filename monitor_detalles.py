#!/usr/bin/env python3
"""
Script mejorado para monitorear logs detallados del ESP32-S3
con el diagnóstico completo de WiFi y MQTT
"""

import serial
import threading
import time
import re
from datetime import datetime

def monitor_esp32_detailed(stop_event):
    """Monitoreo detallado de logs ESP32-S3 con nuevo sistema de logging"""
    try:
        ser = serial.Serial('/dev/ttyACM0', 115200, timeout=0.1)
        time.sleep(0.5)

        print("🔍 MONITOREO DETALLADO DE LOGS ESP32-S3")
        print("=" * 70)
        print("Buscando eventos de WiFi, MQTT, y conexión con nuevo logging...")
        print("Controles: Ctrl+C para detener")
        print()

        # Contadores
        stats = {
            'wifi_start': 0,
            'wifi_got_ip': 0,
            'wifi_disconnected': 0,
            'wifi_retries': 0,
            'mqtt_init': 0,
            'mqtt_connected': 0,
            'mqtt_disconnected': 0,
            'mqtt_published': 0,
            'mqtt_errors': 0,
            'heartbeat_attempts': 0,
            'heartbeat_published': 0,
            'ibeacon_detected': 0
        }

        last_wifi_status = None
        last_mqtt_status = None

        # Esperar inicialización
        time.sleep(2)

        while not stop_event.is_set():
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]

                        # Eventos WiFi
                        if "[WIFI-START]" in line:
                            stats['wifi_start'] += 1
                            print(f"📡 [{timestamp}] WiFi iniciado - {line.split('WiFi STA iniciado')[-1].strip()}")

                        elif "[WIFI-CREDS]" in line:
                            print(f"🔑 [{timestamp}] Credenciales WiFi - {line.split('[WIFI-CREDS]')[-1].strip()}")

                        elif "[WIFI-GOT-IP]" in line:
                            stats['wifi_got_ip'] += 1
                            ip_match = re.search(r'IP Obtenida: (\d+\.\d+\.\d+\.\d+)', line)
                            if ip_match:
                                ip = ip_match.group(1)
                                print(f"🌐 [{timestamp}] ✅ WiFi CONECTADO - IP: {ip}")
                            else:
                                print(f"🌐 [{timestamp}] ✅ WiFi CONECTADO")

                        elif "[WIFI-DISCONNECTED]" in line:
                            stats['wifi_disconnected'] += 1
                            reason_match = re.search(r'Razón: (\d+)', line)
                            reason = reason_match.group(1) if reason_match else "N/A"
                            print(f"❌ [{timestamp}] WiFi DESCONECTADO - Razón: {reason}")

                        elif "[WIFI-RETRY]" in line:
                            stats['wifi_retries'] += 1
                            print(f"🔄 [{timestamp}] Reintento WiFi")

                        elif "[WIFI-CRITICAL]" in line:
                            print(f"💥 [{timestamp}] ERROR CRÍTICO WiFi - {line}")

                        # Eventos MQTT
                        elif "[MQTT-INIT]" in line or "[MQTT-CONFIG]" in line:
                            stats['mqtt_init'] += 1
                            if "Broker URI" in line:
                                print(f"🔗 [{timestamp}] Broker: {line.split('URI:')[-1].strip()}")
                            elif "Username" in line:
                                print(f"👤 [{timestamp}] Usuario: {line.split('Username:')[-1].strip()}")
                            elif "Password length" in line:
                                print(f"🔑 [{timestamp}] Password length: {line.split('length:')[-1].strip()}")

                        elif "[MQTT-HANDLER]" in line:
                            print(f"🔍 [{timestamp}] Evento MQTT handler: {line.split('Event received:')[-1].strip()}")

                        elif "MQTT_EVENT_CONNECTED" in line:
                            stats['mqtt_connected'] += 1
                            print(f"🔗 [{timestamp}] ✅ MQTT CONECTADO!")

                        elif "MQTT_EVENT_DISCONNECTED" in line:
                            stats['mqtt_disconnected'] += 1
                            print(f"❌ [{timestamp}] MQTT DESCONECTADO")

                        elif "MQTT_EVENT_PUBLISHED" in line:
                            stats['mqtt_published'] += 1
                            msg_id_match = re.search(r'msg_id=(\d+)', line)
                            msg_id = msg_id_match.group(1) if msg_id_match else "N/A"
                            print(f"📤 [{timestamp}] MQTT PUBLICADO - ID: {msg_id}")

                        elif "MQTT_EVENT_ERROR" in line:
                            stats['mqtt_errors'] += 1
                            print(f"💥 [{timestamp}] ERROR MQTT - {line}")

                        # Eventos Heartbeat
                        elif "[HEARTBEAT]" in line:
                            if "Counter #" in line and "WiFi:" in line:
                                stats['heartbeat_attempts'] += 1
                                parts = line.split()
                                counter_num = "N/A"
                                wifi_status = "N/A"
                                mqtt_status = "N/A"

                                for part in parts:
                                    if part.startswith("Counter#"):
                                        counter_num = part.replace("#", "")
                                    elif part == "CONNECTED" and parts[parts.index(part)-1] == "WiFi:":
                                        wifi_status = "CONNECTED"
                                    elif part == "DISCONNECTED" and parts[parts.index(part)-1] == "WiFi:":
                                        wifi_status = "DISCONNECTED"

                                print(f"💗 [{timestamp}] Heartbeat #{counter_num} - WiFi: {wifi_status}")

                            elif "QUEUED" in line or "Error queueing" in line:
                                if "QUEUED" in line:
                                    stats['heartbeat_published'] += 1
                                    print(f"✅ [{timestamp}] Heartbeat enviado exitosamente")
                                else:
                                    print(f"❌ [{timestamp}] Error enviando heartbeat")

                        # Eventos iBeacon
                        elif "[MQTT-PUBLISH]" in line and "Publishing to topic" in line:
                            topic = line.split("topic:")[-1].strip()
                            if "ibeacon" in topic.lower():
                                stats['ibeacon_detected'] += 1
                                print(f"🎯 [{timestamp}] iBeacon detectado - Publicando a {topic}")

                        # Status general
                        elif "WiFi connected: YES" in line:
                            print(f"🌐 [{timestamp}] Status WiFi: CONECTADO ✅")

                        elif "WiFi connected: NO" in line:
                            print(f"🌐 [{timestamp}] Status WiFi: DESCONECTADO ❌")

                except Exception as e:
                    pass
            else:
                time.sleep(0.01)

        # Resumen final
        print("\n" + "=" * 70)
        print("📊 RESUMEN DETALLADO DE CONEXIÓN")
        print("=" * 70)

        print(f"📡 WiFi:")
        print(f"   • Inicios WiFi: {stats['wifi_start']}")
        print(f"   • IPs obtenidas: {stats['wifi_got_ip']}")
        print(f"   • Desconexiones: {stats['wifi_disconnected']}")
        print(f"   • Reintentos: {stats['wifi_retries']}")

        print(f"\n🔗 MQTT:")
        print(f"   • Inicializaciones: {stats['mqtt_init']}")
        print(f"   • Conexiones: {stats['mqtt_connected']}")
        print(f"   • Desconexiones: {stats['mqtt_disconnected']}")
        print(f"   • Publicaciones: {stats['mqtt_published']}")
        print(f"   • Errores: {stats['mqtt_errors']}")

        print(f"\n💗 Heartbeat:")
        print(f"   • Intentos: {stats['heartbeat_attempts']}")
        print(f"   • Publicados: {stats['heartbeat_published']}")
        print(f"   • Tasa de éxito: {(stats['heartbeat_published']/max(stats['heartbeat_attempts'],1)*100):.1f}%")

        print(f"\n🎯 iBeacon:")
        print(f"   • Detectados: {stats['ibeacon_detected']}")

        ser.close()

    except Exception as e:
        print(f"❌ Error: {e}")

def main():
    print("🔍 INICIANDO MONITOREO DETALLADO ESP32-S3")
    print("Duración: 90 segundos o Ctrl+C para detener\n")

    stop_event = threading.Event()
    monitor_thread = threading.Thread(target=monitor_esp32_detailed, args=(stop_event,))
    monitor_thread.start()

    try:
        time.sleep(90)
    except KeyboardInterrupt:
        print("\n⏹️  Monitoreo detenido por usuario")
    finally:
        stop_event.set()
        monitor_thread.join()
        print("\n🏁 Monitoreo finalizado")

if __name__ == "__main__":
    main()