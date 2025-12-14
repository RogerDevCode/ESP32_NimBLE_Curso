# CMakeLists.txt del componente "main" para el Proyecto 6.
# Sin cambios en dependencias.

idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES "freertos" "nvs_flash" "bt" "esp_pm")
