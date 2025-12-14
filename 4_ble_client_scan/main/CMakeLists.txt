# CMakeLists.txt del componente "main" para el Proyecto 4.
# Dependencias idénticas al proyecto anterior.

idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES "freertos" "nvs_flash" "bt")
