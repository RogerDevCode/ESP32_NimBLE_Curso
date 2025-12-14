# CMakeLists.txt del componente "main" para el Proyecto 2 - BLE Advertising
# Registra el componente principal con dependencias de FreeRTOS y NimBLE

idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash bt
)
