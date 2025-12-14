#!/usr/bin/env python3
"""
Script para capturar los detalles completos de los errores MQTT
"""

import serial
import threading
import time
from datetime import datetime

def capturar_errores_mqtt(stop_event):
    """Capturar detalles completos de errores MQTT"""
    try:
        ser = serial.Serial('/dev/ttyACM0', 115200, timeout=0.1)
        time.sleep(0.5)

        print("🔍 CAPTURADOR DE ERRORES MQTT DETALLADOS")
        print("=" * 70)
        print("Buscando detalles de errores MQTT_EVENT_ERROR...")
        print()

        # Buffer para capturar mensajes completos
        error_buffer = []
        capturing_error = False

        while not stop_event.is_set():
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]

                        # Iniciar captura cuando detectamos error MQTT
                        if "MQTT_EVENT_ERROR" in line:
                            capturing_error = True
                            error_buffer = []
                            print(f"\n💥 [{timestamp}] === INICIO ERROR MQTT ===")
                            print(f"💥 [{timestamp}] {line}")

                        # Capturar todas las líneas relacionadas con el error
                        elif capturing_error:
                            if "Last error code from esp-tls" in line:
                                error_buffer.append(f"ESP_TLS_ERROR: {line}")
                                print(f"🔒 [{timestamp}] {line}")
                            elif "Last tls stack error" in line:
                                error_buffer.append(f"TLS_STACK_ERROR: {line}")
                                print(f"🔒 [{timestamp}] {line}")
                            elif "Last captured errno" in line:
                                error_buffer.append(f"ERRNO: {line}")
                                print(f"🔒 [{timestamp}] {line}")
                            elif "Broker URI" in line:
                                error_buffer.append(f"BROKER_URI: {line}")
                                print(f"🔗 [{timestamp}] {line}")
                            elif "Username" in line:
                                error_buffer.append(f"USERNAME: {line}")
                                print(f"👤 [{timestamp}] {line}")
                            elif "Unknown error type" in line:
                                error_buffer.append(f"UNKNOWN_TYPE: {line}")
                                print(f"❓ [{timestamp}] {line}")
                            elif "Connection refused error" in line:
                                error_buffer.append(f"REFUSED_ERROR: {line}")
                                print(f"🚫 [{timestamp}] {line}")
                            # Cuando vemos un evento diferente, terminamos la captura
                            elif "MQTT_EVENT_" in line and "MQTT_EVENT_ERROR" not in line:
                                if error_buffer:  # Si tenemos contenido en el buffer
                                    print(f"✅ [{timestamp}] === FIN ERROR MQTT ===\n")
                                    print("📋 RESUMEN DEL ERROR:")
                                    for error_line in error_buffer:
                                        print(f"   • {error_line}")
                                    print()
                                error_buffer = []
                                capturing_error = False

                        # Siempre mostrar eventos de conexión importantes
                        elif "MQTT_EVENT_CONNECTED" in line:
                            print(f"✅ [{timestamp}] 🎉 MQTT CONECTADO!")
                        elif "MQTT_EVENT_DISCONNECTED" in line:
                            print(f"❌ [{timestamp}] MQTT DESCONECTADO")
                        elif "[MQTT-INIT]" in line or "[MQTT-CONFIG]" in line:
                            print(f"⚙️ [{timestamp}] {line}")

                except Exception as e:
                    pass
            else:
                time.sleep(0.01)

        ser.close()

    except Exception as e:
        print(f"❌ Error: {e}")

def main():
    print("🔍 CAPTURANDO ERRORES MQTT DETALLADOS")
    print("Duración: 60 segundos o Ctrl+C para detener\n")

    stop_event = threading.Event()
    capture_thread = threading.Thread(target=capturar_errores_mqtt, args=(stop_event,))
    capture_thread.start()

    try:
        time.sleep(60)
    except KeyboardInterrupt:
        print("\n⏹️  Captura detenida por usuario")
    finally:
        stop_event.set()
        capture_thread.join()
        print("\n🏁 Captura finalizada")

if __name__ == "__main__":
    main()