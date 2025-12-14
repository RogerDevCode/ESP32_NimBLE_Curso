#!/usr/bin/env python3
import os
import sys
import subprocess
import csv

# --- CONFIGURACIÓN DE FÁBRICA ---
# En un entorno real, estos vendrían de una base de datos o input seguro
CREDENTIALS = {
    "wifi_ssid": "Iamtitanium",
    "wifi_pass": "@Roger1357",
    "mqtt_url": "mqtts://53b39b4ca3d649d696d8126555721baf.s1.eu.hivemq.cloud:8883",
    "mqtt_user": "roger-esp32",
    "mqtt_pass": "@Roger1357",
    "mqtt_topic_b": "/concepcion/esp32/ibeacon",
    "mqtt_topic_c": "/concepcion/esp32/contador"
}

IDF_PATH = os.environ.get('IDF_PATH')
if not IDF_PATH:
    # Intento de ruta deducida del entorno actual
    IDF_PATH = "/home/manager/esp/v5.5.1/esp-idf"

NVS_TOOL = os.path.join(IDF_PATH, "components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py")
CSV_FILENAME = "factory_data.csv"
BIN_FILENAME = "factory_data.bin"

def create_csv():
    print(f"Generating {CSV_FILENAME}...")
    with open(CSV_FILENAME, mode='w', newline='') as file:
        writer = csv.writer(file)
        writer.writerow(["key", "type", "encoding", "value"])
        writer.writerow(["gateway_config", "namespace", "", ""]) # Namespace
        
        for key, value in CREDENTIALS.items():
            writer.writerow([key, "data", "string", value])
    print("CSV generated.")

def generate_bin():
    print(f"Generating NVS Binary: {BIN_FILENAME}...")
    # python nvs_partition_gen.py generate factory_data.csv factory_data.bin 0x6000
    cmd = [sys.executable, NVS_TOOL, "generate", CSV_FILENAME, BIN_FILENAME, "0x6000"]
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print("Error generating binary:")
        print(result.stderr)
        sys.exit(1)
    print("Binary generated successfully.")

def flash_bin():
    print("Flashing NVS partition...")
    # idf.py -p /dev/ttyACM0 partition-table
    # Check partitions.csv offset for 'nvs'. Usually 0x9000
    # nvs, data, nvs, 0x9000, 0x6000
    
    cmd = [
        "python3", "-m", "esptool",
        "--chip", "esp32s3",
        "-p", "/dev/ttyACM0",
        "-b", "460800",
        "--before", "default_reset",
        "--after", "hard_reset",
        "write_flash",
        "0x9000", BIN_FILENAME 
    ]
    
    result = subprocess.run(cmd)
    if result.returncode != 0:
        print("Error flashing NVS.")
        sys.exit(1)
    print("Device provisioned successfully!")

if __name__ == "__main__":
    if not os.path.exists(NVS_TOOL):
        print(f"Error: NVS Tool not found at {NVS_TOOL}")
        sys.exit(1)
        
    create_csv()
    generate_bin()
    flash_bin()
    
    # Cleanup
    if os.path.exists(CSV_FILENAME): os.remove(CSV_FILENAME)
    if os.path.exists(BIN_FILENAME): os.remove(BIN_FILENAME)
