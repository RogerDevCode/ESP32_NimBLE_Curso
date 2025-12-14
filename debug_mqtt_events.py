#!/usr/bin/env python3
"""
Script para diagnosticar eventos MQTT reales y conexión al broker
"""

import serial
import threading
import time
import json
from datetime import datetime

def debug_mqtt_events(stop_event):
    """Monitorear eventos MQTT detallados"""
    try:
        ser = serial.Serial('/dev/ttyACM0', 115200, timeout=0.1)
        time.sleep(0.5)

        print("🔍 DIAGNÓSTICO DETALLADO DE EVENTOS MQTT")
        print("=" * 60)
        print("Buscando eventos MQTT_EVENT_ completos...")
        print("Broker: mqtts://53b39b4ca3d649d696d8126555721baf.s1.eu.hivemq.cloud:8883")
        print("Usuario: roger-esp32")
        print()

        eventos_mqtt = []
        heartbeat_enviados = 0
        heartbeat_publicados = 0

        while not stop_event.is_set():
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]

                        # Eventos MQTT exactos
                        if "MQTT_EVENT_CONNECTED" in line:
                            eventos_mqtt.append(("CONNECTED", timestamp))
                            print(f"✅ [{timestamp}] MQTT_EVENT_CONNECTED - ¡Conexión establecida!")

                        elif "MQTT_EVENT_DISCONNECTED" in line:
                            eventos_mqtt.append(("DISCONNECTED", timestamp))
                            print(f"❌ [{timestamp}] MQTT_EVENT_DISCONNECTED - ¡Conexión perdida!")

                        elif "MQTT_EVENT_ERROR" in line:
                            eventos_mqtt.append(("ERROR", timestamp))
                            print(f"💥 [{timestamp}] MQTT_EVENT_ERROR - {line}")

                        elif "MQTT_EVENT_PUBLISHED" in line:
                            heartbeat_publicados += 1
                            eventos_mqtt.append(("PUBLISHED", timestamp))
                            # Extraer msg_id
                            if "msg_id=" in line:
                                msg_id = line.split("msg_id=")[1]
                                print(f"📤 [{timestamp}] MQTT_EVENT_PUBLISHED - msg_id={msg_id} ¡Confirmación de entrega!")

                        elif "Error MQTT" in line or "Error publicar MQTT" in line:
                            print(f"❌ [{timestamp}] {line}")

                        # Eventos de nuestro heartbeat
                        elif "Heartbeat #" in line and "enviado a" in line:
                            heartbeat_enviados += 1
                            print(f"💗 [{timestamp}] Intento de envío #{heartbeat_enviados}")

                        # Información general del cliente MQTT
                        elif "Cliente MQTT" in line:
                            print(f"🔧 [{timestamp}] {line}")

                        # Errores TLS o de red
                        elif "TLS" in line or "tls" in line:
                            print(f"🔒 [{timestamp}] TLS: {line}")

                        elif "transport" in line and "Error" in line:
                            print(f"🚨 [{timestamp}] Transport Error: {line}")

                except Exception as e:
                    pass
            else:
                time.sleep(0.01)

        # Resumen final
        print("\n" + "=" * 60)
        print("📊 RESUMEN DE DIAGNÓSTICO MQTT")
        print("=" * 60)

        connected_events = [e for e in eventos_mqtt if e[0] == "CONNECTED"]
        disconnected_events = [e for e in eventos_mqtt if e[0] == "DISCONNECTED"]
        error_events = [e for e in eventos_mqtt if e[0] == "ERROR"]
        published_events = [e for e in eventos_mqtt if e[0] == "PUBLISHED"]

        print(f"❤️  Heartbeat intentados: {heartbeat_enviados}")
        print(f"📤 MQTT_EVENT_PUBLISHED: {heartbeat_publicados}")
        print(f"🔗 MQTT_EVENT_CONNECTED: {len(connected_events)}")
        print(f"❌ MQTT_EVENT_DISCONNECTED: {len(disconnected_events)}")
        print(f"💥 MQTT_EVENT_ERROR: {len(error_events)}")

        print(f"\n🔍 ANÁLISIS:")
        if heartbeat_enviados > 0 and heartbeat_publicados == 0:
            print("❌ PROBLEMA CRÍTICO: Los heartbeats se intentan enviar pero nunca se publican")
            print("   → Causa posible: MQTT no está realmente conectado al broker")

        if len(connected_events) == 0:
            print("❌ MQTT nunca se conectó al broker")
        elif len(connected_events) > 0 and len(disconnected_events) > 0:
            print("⚠️ MQTT se conectó pero luego se desconectó")

        ser.close()

    except Exception as e:
        print(f"❌ Error: {e}")

def main():
    print("🔍 INICIANDO DIAGNÓSTICO MQTT DETALLADO")
    print("Duración: 45 segundos para capturar eventos completos\n")

    stop_event = threading.Event()
    monitor_thread = threading.Thread(target=debug_mqtt_events, args=(stop_event,))
    monitor_thread.start()

    try:
        time.sleep(45)
    except KeyboardInterrupt:
        print("\n⏹️  Diagnóstico detenido por usuario")
    finally:
        stop_event.set()
        monitor_thread.join()
        print("\n🏁 Diagnóstico finalizado")

if __name__ == "__main__":
    main()