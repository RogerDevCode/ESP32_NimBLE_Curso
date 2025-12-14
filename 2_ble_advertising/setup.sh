#!/bin/bash
# Script de configuración rápida para el proyecto 2_ble_advertising
# Ejecuta este script para configurar el entorno ESP-IDF

# Colores para output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== Configuración del Proyecto BLE Advertising ===${NC}"

# 1. Configurar el entorno ESP-IDF
echo -e "${YELLOW}1. Configurando entorno ESP-IDF...${NC}"
. /home/manager/esp/v5.5.1/esp-idf/export.sh

# 2. Establecer el target (solo necesario la primera vez)
echo -e "${YELLOW}2. Estableciendo target ESP32...${NC}"
idf.py set-target esp32

# 3. Compilar el proyecto
echo -e "${YELLOW}3. Compilando el proyecto...${NC}"
idf.py build

echo -e "${GREEN}=== Configuración completada ===${NC}"
echo ""
echo "Para flashear el dispositivo, ejecuta:"
echo "  idf.py -p /dev/ttyUSB0 flash"
echo ""
echo "Para monitorear la salida, ejecuta:"
echo "  idf.py -p /dev/ttyUSB0 monitor"
echo ""
echo "Para flashear y monitorear en un solo comando:"
echo "  idf.py -p /dev/ttyUSB0 flash monitor"
