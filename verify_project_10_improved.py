#!/usr/bin/env python3
"""
Script de Verificación Mejorado para Proyecto 10 (BLE MQTT Gateway)
Cumple con la solicitud de 'logs atómicos' y verificación de funcionamiento.
"""

import serial
import time
import sys
import glob
from datetime import datetime
import re

# Configuración esperada (debe coincidir con main.c / sdkconfig)
EXPECTED_TOPIC_BEACON = "/concepcion/esp32/ibeacon"
EXPECTED_TOPIC_COUNTER = "/concepcion/esp32/contador"

class AtomicLogger:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

    @staticmethod
    def log(category, message, level="INFO"):
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        color = AtomicLogger.OKBLUE
        icon = "ℹ️"
        
        if level == "SUCCESS":
            color = AtomicLogger.OKGREEN
            icon = "✅"
        elif level == "WARNING":
            color = AtomicLogger.WARNING
            icon = "⚠️"
        elif level == "ERROR":
            color = AtomicLogger.FAIL
            icon = "❌"
        elif level == "EVENT":
            color = AtomicLogger.OKCYAN
            icon = "⚡"

        print(f"{AtomicLogger.BOLD}[{timestamp}]{AtomicLogger.ENDC} {color}[{category}]{AtomicLogger.ENDC} {icon} {message}")

def find_esp_port():
    ports = glob.glob('/dev/ttyUSB*') + glob.glob('/dev/ttyACM*')
    if not ports:
        return None
    # Prefer ACM0 for S3, USB0 for standard
    for p in ports:
        if "ACM" in p: return p
    return ports[0]

def verify_functioning():
    port = find_esp_port()
    if not port:
        AtomicLogger.log("SYSTEM", "No se encontró puerto serial (ttyUSB/ttyACM)", "ERROR")
        return

    AtomicLogger.log("SYSTEM", f"Conectando a {port}...", "INFO")
    
    try:
        ser = serial.Serial(port, 115200, timeout=0.1)
        
        # Reset board
        ser.dtr = False
        ser.rts = True
        time.sleep(0.1)
        ser.rts = False
        time.sleep(0.5)
        
        AtomicLogger.log("SYSTEM", "ESP32 Reiniciado. Escuchando...", "INFO")
        
        start_time = time.time()
        wifi_connected = False
        mqtt_connected = False
        heartbeat_seen = False
        
        while time.time() - start_time < 60: # 60 segundos de prueba
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if not line: continue
                    
                    # Parsing Log Logic from main.c
                    
                    # WiFi
                    if "[WIFI-GOT-IP]" in line:
                        AtomicLogger.log("WIFI", "IP Obtenida. Conexión Exitosa.", "SUCCESS")
                        wifi_connected = True
                    elif "WIFI_EVENT_STA_DISCONNECTED" in line:
                        AtomicLogger.log("WIFI", "Desconectado. Reintentando...", "WARNING")
                    
                    # MQTT Connection
                    elif "MQTT_EVENT_CONNECTED" in line:
                        AtomicLogger.log("MQTT", "Conectado al Broker HiveMQ", "SUCCESS")
                        mqtt_connected = True
                    elif "MQTT_EVENT_ERROR" in line:
                         AtomicLogger.log("MQTT", f"Error de conexión: {line}", "ERROR")

                    # MQTT Publish (Beacon)
                    elif "[MQTT-PUBLISH]" in line:
                        if EXPECTED_TOPIC_BEACON in line:
                            AtomicLogger.log("PUB", f"Beacon publicado en {EXPECTED_TOPIC_BEACON}", "SUCCESS")
                        else:
                            AtomicLogger.log("PUB", f"Publicado en tópico desconocido: {line}", "WARNING")

                    # Heartbeat
                    elif "[HEARTBEAT]" in line and "Publishing heartbeat" in line:
                        if EXPECTED_TOPIC_COUNTER in line:
                             AtomicLogger.log("HEARTBEAT", f"Contador enviado a {EXPECTED_TOPIC_COUNTER}", "SUCCESS")
                             heartbeat_seen = True
                        else:
                             AtomicLogger.log("HEARTBEAT", "Contador enviado a tópico incorrecto", "WARNING")
                    
                    # Watchdog / System
                    elif "Guru Meditation Error" in line:
                        AtomicLogger.log("CRASH", f"PANIC DETECTADO: {line}", "ERROR")

                except Exception as e:
                    pass
            else:
                time.sleep(0.01)
                
        # Resumen
        print("\n" + "="*50)
        AtomicLogger.log("RESULT", "Resumen de Verificación (60s):", "INFO")
        AtomicLogger.log("CHECK", f"WiFi Conectado: {'PASS' if wifi_connected else 'FAIL'}", "SUCCESS" if wifi_connected else "ERROR")
        AtomicLogger.log("CHECK", f"MQTT Conectado: {'PASS' if mqtt_connected else 'FAIL'}", "SUCCESS" if mqtt_connected else "ERROR")
        AtomicLogger.log("CHECK", f"Heartbeat:      {'PASS' if heartbeat_seen else 'FAIL'}", "SUCCESS" if heartbeat_seen else "WARNING")
        print("="*50 + "\n")

    except serial.SerialException as e:
        AtomicLogger.log("SYSTEM", f"Error abriendo puerto: {e}", "ERROR")
    except KeyboardInterrupt:
        AtomicLogger.log("SYSTEM", "Detenido por usuario", "WARNING")

if __name__ == "__main__":
    verify_functioning()
