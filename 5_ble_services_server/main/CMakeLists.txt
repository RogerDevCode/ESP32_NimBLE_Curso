# CMakeLists.txt del componente "main" para el Proyecto 3.
# No hay cambios en las dependencias con respecto al proyecto anterior.

idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES "freertos" "bt" "nvs_flash")
