#!/usr/bin/env python3
"""
Script específico para capturar la razón de desconexión WiFi
"""

import serial
import time
import sys

def main():
    try:
        ser = serial.Serial('/dev/ttyACM0', 115200, timeout=0.1)

        # Reset
        ser.dtr = False
        ser.rts = True
        time.sleep(0.1)
        ser.rts = False
        time.sleep(1.0)

        print("🔍 ANÁLISIS DE RAZÓN DE DESCONEXIÓN WiFi")
        print("=" * 50)
        print("Esperando eventos WiFi...")

        # Esperar a que el sistema inicie
        time.sleep(5)

        start_time = time.time()
        timeout = 30  # segundos

        while time.time() - start_time < timeout:
            if ser.in_waiting > 0:
                try:
                    line = ser.readline().decode('utf-8', errors='ignore').strip()
                    if line:
                        # Buscar eventos clave
                        if "WiFi STA iniciado" in line:
                            print(f"📡 {line}")
                        elif "Intentando conectar a" in line:
                            print(f"🎯 {line}")
                        elif "desconectado. Razón:" in line:
                            print(f"❌ {line}")
                            # Extraer número de razón
                            import re
                            reason_match = re.search(r'Razón: (\d+)', line)
                            if reason_match:
                                reason_code = int(reason_match.group(1))
                                print(f"   Código de razón: {reason_code}")
                        elif "SSID:" in line:
                            print(f"📶 {line}")
                        elif "IP Obtenida:" in line:
                            print(f"🌐 {line}")
                            print("✅ ¡WiFi CONECTADO!")
                            ser.close()
                            return 0
                        elif "GATEWAY_PRO" in line and ("desconectado" in line or "WiFi" in line):
                            print(f"🔍 {line}")

                except Exception as e:
                    pass
            else:
                time.sleep(0.01)

        print("\n⏱️ Timeout - no se obtuvo conexión en 30 segundos")
        ser.close()
        return 1

    except Exception as e:
        print(f"❌ Error: {e}")
        return 1

if __name__ == "__main__":
    sys.exit(main())