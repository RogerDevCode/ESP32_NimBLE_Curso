# Este es el archivo CMakeLists.txt principal del proyecto.
# Especifica la versión mínima de CMake y el nombre del proyecto.

cmake_minimum_required(VERSION 3.16)

# Incluye la funcionalidad del build system de ESP-IDF.
# Esto debe estar antes de la declaración del proyecto.
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Declara el nombre del proyecto.
project(1_hola_mundo)
