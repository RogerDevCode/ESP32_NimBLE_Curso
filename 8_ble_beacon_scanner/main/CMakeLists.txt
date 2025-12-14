# CMakeLists.txt del componente "main" para el Proyecto 8.

idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES "freertos" "bt" "nvs_flash")
