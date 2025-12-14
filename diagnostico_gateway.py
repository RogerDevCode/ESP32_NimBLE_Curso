#!/usr/bin/env python3
"""
Script de Diagnóstico Específico para Gateway BLE-MQTT
Diferencia entre problemas de WiFi vs MQTT
"""

import serial
import threading
import time
import re
from datetime import datetime

def diagnostic_acm0(stop_event):
    """Diagnóstico detallado del puerto ACM0"""
    try:
        ser = serial.Serial('/dev/ttyACM0', 115200, timeout=0.1)

        # Reset
        ser.dtr = False
        ser.rts = True
        time.sleep(0.1)
        ser.rts = False
        time.sleep(0.5)

        print("🔍 DIAGNÓSTICO GATEWAY BLE-MQTT")
        print("=" * 50)
        print("ESP32-S3 en /dev/ttyACM0")
        print("Buscando eventos específicos de WiFi y MQTT...")
        print()

        wifi_connected = False
        ip_obtenida = False
        mqtt_started = False
        mqtt_error_dns = False
        mqtt_connected = False
        ble_scanning = False

        while not stop_event.is_set():
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]

                        # ANÁLISIS DE EVENTOS WIFI
                        if "wifi:mode : sta" in line:
                            print(f"📡 [{timestamp}] [WIFI_INIT] Modo Station configurado")

                        elif "wifi:enable tsf" in line:
                            print(f"📡 [{timestamp}] [WIFI_INIT] TSF habilitado")

                        elif "IP Obtenida:" in line:
                            ip_obtenida = True
                            wifi_connected = True
                            # Extraer IP con regex
                            ip_match = re.search(r'(\d+\.\d+\.\d+\.\d+)', line)
                            ip = ip_match.group(1) if ip_match else "desconocida"
                            print(f"🌐 [{timestamp}] [WIFI_OK] ✓ WiFi CONECTADO - IP: {ip}")

                        elif "WiFi desconectado. Reintentando en" in line:
                            wifi_connected = False
                            print(f"❌ [{timestamp}] [WIFI_FAIL] WiFi desconectado - Reintentando...")

                        elif "Fallo crítico de WiFi" in line:
                            print(f"💀 [{timestamp}] [WIFI_CRITICAL] Fallo crítico - Sistema reiniciará")

                        # ANÁLISIS DE EVENTOS MQTT
                        elif "broker.hivemq.com" in line and "getaddrinfo" in line:
                            mqtt_error_dns = True
                            print(f"🔍 [{timestamp}] [MQTT_DNS] Intentando resolver broker.hivemq.com...")

                        elif "couldn't get hostname for :broker.hivemq.com" in line:
                            print(f"❌ [{timestamp}] [MQTT_DNS_FAIL] ERROR DNS - No puede resolver broker.hivemq.com")
                            print(f"   → Causa: WiFi no conectado o sin internet")

                        elif "mqtt_client: Error transport connect" in line:
                            print(f"❌ [{timestamp}] [MQTT_CONNECT_FAIL] Error conexión MQTT")

                        elif "Publicado:" in line and "gateway/data" in line:
                            mqtt_connected = True
                            print(f"📤 [{timestamp}] [MQTT_OK] ✓ Publicación exitosa a MQTT")

                        # ANÁLISIS DE EVENTOS BLE
                        elif "BLE Sincronizado. Iniciando escaneo..." in line:
                            ble_scanning = True
                            print(f"🔵 [{timestamp}] [BLE_OK] ✓ BLE activo - Escaneo iniciado")

                        elif "iBeacon DETECTED!" in line:
                            # Extraer MAC y RSSI
                            mac_match = re.search(r'Addr: ([0-9a-f:]+)', line)
                            rssi_match = re.search(r'RSSI: (-?\d+)', line)
                            mac = mac_match.group(1) if mac_match else "unknown"
                            rssi = rssi_match.group(1) if rssi_match else "unknown"
                            print(f"🎯 [{timestamp}] [iBEACON] Detectado - MAC: {mac} | RSSI: {rssi}")

                        elif "WiFi no conectado. Descartando paquete BLE." in line:
                            print(f"⚠️  [{timestamp}] [BLE_DROP] Paquete descartado - WiFi no conectado")

                        # ESTADO GENERAL
                        elif "app_init: Project name:" in line:
                            print(f"🚀 [{timestamp}] [SYSTEM] Iniciando Gateway BLE-MQTT...")

                        elif "main_task: Returned from app_main()" in line:
                            print(f"✅ [{timestamp}] [SYSTEM] Sistema inicializado - Entrando en modo operativo")

                except Exception as e:
                    print(f"[ERROR] Decodificando: {e}")
            else:
                time.sleep(0.01)

        # RESUMEN FINAL
        print("\n" + "=" * 50)
        print("📊 RESUMEN DE DIAGNÓSTICO")
        print("=" * 50)
        print(f"🌐 WiFi Conectado: {'✅ SÍ' if wifi_connected else '❌ NO'}")
        print(f"📤 IP Obtenida:    {'✅ SÍ' if ip_obtenida else '❌ NO'}")
        print(f"🔵 BLE Activo:     {'✅ SÍ' if ble_scanning else '❌ NO'}")
        print(f"📡 MQTT Conectado: {'✅ SÍ' if mqtt_connected else '❌ NO'}")
        print(f"🔍 Error DNS MQTT: {'✅ SÍ' if mqtt_error_dns else '❌ NO'}")

        print("\n💡 DIAGNÓSTICO:")
        if not wifi_connected:
            print("❌ PROBLEMA: WiFi no se conecta a la red 'Iamtitanium'")
            print("   → Verificar: SSID, contraseña, disponibilidad de red")
        elif not ip_obtenida:
            print("⚠️  WiFi conectado pero sin IP (problema DHCP)")
        elif mqtt_error_dns and not mqtt_connected:
            print("❌ PROBLEMA: WiFi conectado pero sin acceso a internet")
            print("   → No puede resolver broker.hivemq.com")
        elif mqtt_connected:
            print("✅ SISTEMA FUNCIONANDO PERFECTAMENTE")

        ser.close()

    except Exception as e:
        print(f"❌ Error conectando a /dev/ttyACM0: {e}")

def main():
    print("🔍 INICIANDO DIAGNÓSTICO ESPECÍFICO")
    print("Monitoreo por 60 segundos para análisis detallado...\n")

    stop_event = threading.Event()
    monitor_thread = threading.Thread(target=diagnostic_acm0, args=(stop_event,))
    monitor_thread.start()

    try:
        time.sleep(60)
    except KeyboardInterrupt:
        print("\n⏹️  Diagnóstico detenido por usuario")
    finally:
        stop_event.set()
        monitor_thread.join()
        print("\n🏁 Diagnóstico finalizado")

if __name__ == "__main__":
    main()