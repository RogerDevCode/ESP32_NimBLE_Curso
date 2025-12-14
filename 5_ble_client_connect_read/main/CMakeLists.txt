# CMakeLists.txt del componente "main" para el Proyecto 5.
# Sin cambios en dependencias.

idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES "freertos" "esp_timer" "bt" "nvs_flash")
