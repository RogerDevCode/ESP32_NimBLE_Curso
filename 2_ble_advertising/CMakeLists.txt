# Archivo CMakeLists.txt principal para el Proyecto 2 - BLE Advertising
# Este archivo configura el proyecto ESP-IDF para trabajar con NimBLE

# Versión mínima requerida de CMake para ESP-IDF
cmake_minimum_required(VERSION 3.16)

# Incluir el sistema de proyecto de ESP-IDF
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Nombre del proyecto
project(2_ble_advertising)
