idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES "nvs_flash" "esp_wifi" "esp_event" "mqtt" "json" "bt"
                    EMBED_FILES "hivemq_cloud_cert.pem")
