# Este es el CMakeLists.txt del componente "main".
# Define los archivos fuente del componente y sus dependencias.

idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES "freertos" "esp_driver_gpio" "log")
