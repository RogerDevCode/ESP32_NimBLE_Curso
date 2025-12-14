#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_timer.h" // Explicitly included for esp_timer_get_time()
#include "esp_netif.h" // Explicitly included for esp_netif_init()
#include "mqtt_client.h"
#include "cJSON.h"
#include "sdkconfig.h"

// NimBLE
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "esp_task_wdt.h" // Added for watchdog
#include "esp_mac.h" // Added for MACSTR and MAC2STR
#include "esp_tls.h"  // Added for TLS support
#include "esp_crt_bundle.h" // <--- OBLIGATORIO PARA HIVEMQ CLOUD

/* --- CONFIGURACIÓN (Vía NVS) --- */
// Global buffers for runtime configuration
static char WIFI_SSID[33] = {0};
static char WIFI_PASS[65] = {0};
static char MQTT_BROKER_URI[128] = {0};
static char MQTT_USERNAME[64] = {0};
static char MQTT_PASSWORD[64] = {0};
static char MQTT_TOPIC_BEACON[64] = {0};
static char MQTT_TOPIC_COUNTER[64] = {0};

#define COUNTER_INTERVAL_MS CONFIG_GATEWAY_COUNTER_INTERVAL_MS

/* --- TLS Certificate --- */
extern const uint8_t hivemq_cloud_cert_pem_start[]   asm("_binary_hivemq_cloud_cert_pem_start");
extern const uint8_t hivemq_cloud_cert_pem_end[]     asm("_binary_hivemq_cloud_cert_pem_end");

static const char *TAG = "GATEWAY_PRO";

/* --- NVS CONFIG LOAD --- */
static void load_config_from_nvs(void)
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("gateway_config", NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "💥 [NVS] Error opening NVS handle! (%s)", esp_err_to_name(err));
        ESP_LOGE(TAG, "💥 [NVS] Did you provision the device?");
        // Safety Halt: Cannot proceed without config
        while(1) { vTaskDelay(1000 / portTICK_PERIOD_MS); }
    }

    size_t required_size;

    // Helper macro for reading string
    #define READ_NVS_STR(key, buffer) \
        required_size = sizeof(buffer); \
        err = nvs_get_str(my_handle, key, buffer, &required_size); \
        if (err == ESP_OK) { \
            ESP_LOGI(TAG, "🔑 [NVS] Loaded %s: %s", key, buffer); \
        } else { \
            ESP_LOGE(TAG, "💥 [NVS] Failed to load %s (%s)", key, esp_err_to_name(err)); \
        }

    READ_NVS_STR("wifi_ssid", WIFI_SSID);
    READ_NVS_STR("wifi_pass", WIFI_PASS);
    READ_NVS_STR("mqtt_url", MQTT_BROKER_URI);
    READ_NVS_STR("mqtt_user", MQTT_USERNAME);
    READ_NVS_STR("mqtt_pass", MQTT_PASSWORD);
    READ_NVS_STR("mqtt_topic_b", MQTT_TOPIC_BEACON);
    READ_NVS_STR("mqtt_topic_c", MQTT_TOPIC_COUNTER);

    nvs_close(my_handle);
}

/* --- FREERTOS OBJECTS --- */
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

// Cola para desacoplar BLE (Productor) de MQTT (Consumidor)
typedef struct {
    char device_name[32];
    int rssi;
    uint8_t addr[6];
    uint8_t uuid[16]; // iBeacon UUID
    uint16_t major;   // iBeacon Major
    uint16_t minor;   // iBeacon Minor
    int8_t tx_power;  // iBeacon Tx Power
} ble_msg_t;

static QueueHandle_t ble_evt_queue = NULL;

/* --- WIFI & MQTT HANDLERS --- */
static esp_mqtt_client_handle_t mqtt_client = NULL;
static int retry_num = 0;

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    ESP_LOGI(TAG, "📡 [WIFI-EVENT] Event: %s, ID: %ld", event_base, event_id);

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "📡 [WIFI-START] WiFi STA iniciado. Intentando conectar a: %s", WIFI_SSID);
        ESP_LOGI(TAG, "🔑 [WIFI-CREDS] SSID: '%s', Pass length: %zu", WIFI_SSID, strlen(WIFI_PASS));
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t* disconnected = (wifi_event_sta_disconnected_t*) event_data;
        ESP_LOGW(TAG, "📡 [WIFI-DISCONNECTED] WiFi desconectado. Razón: %d", disconnected->reason);
        ESP_LOGW(TAG, "📡 [WIFI-DISCONNECTED] SSID: %.32s, RSSI: %d", disconnected->ssid, disconnected->rssi);
        ESP_LOGW(TAG, "📡 [WIFI-DISCONNECTED] Retry count: %d/10", retry_num);

        // Exponential Backoff: Start at 5s, max 60s
        if (retry_num < 10) {
             retry_num++;
        }
        // Base 5s. Shift: 0->5s, 1->10s, 2->20s, 3->40s, 4+->60s
        int delay_ms = 5000 * (1 << (retry_num > 0 ? retry_num - 1 : 0)); 
        if (delay_ms > 60000) delay_ms = 60000;
        
        ESP_LOGW(TAG, "🔄 [WIFI-RETRY] WiFi desconectado (Intento %d). Reintentando en %d ms...", retry_num, delay_ms);
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
        esp_wifi_connect();

        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "🌐 [WIFI-GOT-IP] ✅ IP Obtenida: " IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "🌐 [WIFI-GOT-IP] Netmask: " IPSTR, IP2STR(&event->ip_info.netmask));
        ESP_LOGI(TAG, "🌐 [WIFI-GOT-IP] Gateway: " IPSTR, IP2STR(&event->ip_info.gw));
        retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

        // Log WiFi status immediately
        ESP_LOGI(TAG, "🔥 [WIFI-STATUS] WiFi BIT SET - WiFi connected flag active");
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGI(TAG, "🔍 [MQTT-HANDLER] Event received: %d", event_id);
    esp_mqtt_event_handle_t event = event_data;
    
    // Check WiFi status at every MQTT event
    EventBits_t wifi_bits = xEventGroupGetBits(s_wifi_event_group);
    ESP_LOGI(TAG, "🌐 [WIFI-STATUS] WiFi connected: %s",
             (wifi_bits & WIFI_CONNECTED_BIT) ? "YES ✅" : "NO ❌");

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "✅ MQTT_EVENT_CONNECTED - Conexión exitosa!");
        ESP_LOGI(TAG, "   Client ID: %s", MQTT_USERNAME);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG, "❌ MQTT_EVENT_DISCONNECTED - Desconectado");
        break;

    case MQTT_EVENT_BEFORE_CONNECT:
        ESP_LOGI(TAG, "🔗 MQTT_EVENT_BEFORE_CONNECT");
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "📤 MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "💥 MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            ESP_LOGE(TAG, "   Last error code from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
            ESP_LOGE(TAG, "   Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
            ESP_LOGE(TAG, "   Last captured errno : %d (%s)",  event->error_handle->esp_transport_sock_errno,
                     strerror(event->error_handle->esp_transport_sock_errno));
            ESP_LOGE(TAG, "   Broker URI: %s", MQTT_BROKER_URI);
            ESP_LOGE(TAG, "   Username: %s", MQTT_USERNAME);
        } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
            ESP_LOGE(TAG, "   Connection refused error: 0x%x", event->error_handle->connect_return_code);
        } else {
            ESP_LOGW(TAG, "   Unknown error type: 0x%x", event->error_handle->error_type);
        }
        break;

    default:
        ESP_LOGI(TAG, "Other MQTT event id:%d", event->event_id);
        break;
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };

    // Copy from NVS buffers to WiFi Config
    memcpy(wifi_config.sta.ssid, WIFI_SSID, sizeof(wifi_config.sta.ssid));
    memcpy(wifi_config.sta.password, WIFI_PASS, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // CRITICAL: Disable WiFi Power Save for stable MQTT/TLS
    esp_wifi_set_ps(WIFI_PS_NONE);
}

static void mqtt_app_start(void)
{
    ESP_LOGI(TAG, "🚀 [MQTT-INIT] Iniciando cliente MQTT con Seguridad TLS (Obligatorio)...");

    // Validar que la URI empiece por mqtts:// (Capa 8 Check)
    if (strncmp(MQTT_BROKER_URI, "mqtts://", 8) != 0) {
        ESP_LOGW(TAG, "⚠️ [CONFIG-WARNING] La URI configurada no tiene 'mqtts://'. HiveMQ Cloud REQUIERE encriptación.");
        // Nota: Si en tu menuconfig pusiste solo la URL sin el esquema, el código de abajo lo arregla forzando puerto y transporte.
    }

    ESP_LOGI(TAG, "🔗 [MQTT-CONFIG] Broker URI: %s", MQTT_BROKER_URI);
    ESP_LOGI(TAG, "👤 [MQTT-CONFIG] Username: %s", MQTT_USERNAME);
    ESP_LOGI(TAG, "🔑 [MQTT-CONFIG] Password length: %zu", strlen(MQTT_PASSWORD));
    ESP_LOGI(TAG, "📝 [MQTT-CONFIG] Client ID: NULL (Auto-generado por ESP-IDF)");

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = MQTT_BROKER_URI,
            .address.port = 8883, // Forzamos puerto seguro
            .verification = {
                // Esta es la MAGIA que te falta.
                // Usa los certificados raíz integrados en el ESP32 para validar a HiveMQ.
                .crt_bundle_attach = esp_crt_bundle_attach,
            }
        },
        .credentials = {
            .username = MQTT_USERNAME,
            .authentication.password = MQTT_PASSWORD,
            .client_id = NULL, // Dejar NULL es correcto para evitar colisiones
        },
        .session = {
            .keepalive = 60, // 60 segundos es estándar para estabilidad
            .disable_clean_session = 0, // 0 = Sesión persistente (recibe mensajes perdidos)
        }
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "❌ [MQTT-INIT] Fallo crítico: Memoria insuficiente para cliente MQTT");
        return;
    }

    // Registrar eventos
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    ESP_LOGI(TAG, "🔐 [MQTT-SEC] Conectando a HiveMQ Cloud (Port 8883 + SSL)...");

    esp_err_t ret = esp_mqtt_client_start(mqtt_client);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "❌ [MQTT-INIT] Error iniciando tarea del cliente: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "✅ [MQTT-INIT] Cliente MQTT iniciado exitosamente");
    }
}

/* --- BLE HANDLERS --- */
// iBeacon Constants for filtering
#define IBEACON_MFG_ID 0x004C // Apple Inc.

static int ble_gap_event(struct ble_gap_event *event, void *arg)
{
    struct ble_hs_adv_fields fields;
    int rc;

    switch (event->type) {
    case BLE_GAP_EVENT_DISC:
        rc = ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data);
        if (rc != 0) return 0;

        // Filter for iBeacon
        if (fields.mfg_data != NULL && fields.mfg_data_len == 25) {
            uint16_t mfg_id = fields.mfg_data[0] | (fields.mfg_data[1] << 8);
            if (mfg_id == IBEACON_MFG_ID && fields.mfg_data[2] == 0x02 && fields.mfg_data[3] == 0x15) {
                // This is an iBeacon!
                ble_msg_t msg;
                memset(&msg, 0, sizeof(msg));
                
                // Copy iBeacon data
                memcpy(msg.uuid, &fields.mfg_data[4], 16);
                msg.major = __builtin_bswap16(*(uint16_t *)&fields.mfg_data[20]);
                msg.minor = __builtin_bswap16(*(uint16_t *)&fields.mfg_data[22]);
                msg.tx_power = fields.mfg_data[24];

                // Copy other data
                msg.rssi = event->disc.rssi;
                memcpy(msg.addr, event->disc.addr.val, 6);

                // Try to get device name if available (from AdvData or ScanResponse)
                if (fields.name_len > 0) {
                    int len = fields.name_len < sizeof(msg.device_name) - 1 ? fields.name_len : sizeof(msg.device_name) - 1;
                    memcpy(msg.device_name, fields.name, len);
                    msg.device_name[len] = '\0';
                } else {
                    strcpy(msg.device_name, "iBeacon"); // Default name
                }

                // Precision Mode: Dropping packet if queue is almost full to preserve newer data or critical messages
                if (uxQueueSpacesAvailable(ble_evt_queue) < 2) {
                     return 0; // Drop packet to prevent blocking/overflow if consumer is slow
                }

                ESP_LOGI(TAG, "iBeacon DETECTED! Addr: " MACSTR " | RSSI: %d", MAC2STR(msg.addr), msg.rssi);

                // Enviar a la cola (No bloquear si está llena)
                if (xQueueSend(ble_evt_queue, &msg, 0) != pdTRUE) {
                    ESP_LOGW(TAG, "Queue Full - Dropping BLE iBeacon packet from " MACSTR, MAC2STR(msg.addr));
                }
            }
        }
        return 0;
    }
    return 0;
}

static void ble_app_scan(void)
{
    struct ble_gap_disc_params disc_params;
    disc_params.filter_duplicates = 0; // Queremos actualizaciones continuas de RSSI
    disc_params.passive = 1;
    // Coexistence Optimization: 50% Duty Cycle
    // Interval: 160 * 0.625ms = 100ms
    // Window: 80 * 0.625ms = 50ms
    disc_params.itvl = 160; 
    disc_params.window = 80;
    disc_params.filter_policy = 0;
    disc_params.limited = 0;

    ble_gap_disc(BLE_OWN_ADDR_PUBLIC, BLE_HS_FOREVER, &disc_params, ble_gap_event, NULL);
}

static void ble_app_on_sync(void)
{
    ESP_LOGI(TAG, "BLE Sincronizado. Iniciando escaneo...");
    ble_app_scan();
}

static void host_task(void *param)
{
    // nimble_port_run() blocks forever. We cannot easily feed the WDT here.
    // Therefore, we do not add this system task to the Task WDT.
    nimble_port_run();
    nimble_port_freertos_deinit();
}

/* --- TAREA CONSUMIDORA (MQTT PUBLISHER) --- */
void mqtt_publisher_task(void *pvParameters)
{
    ble_msg_t msg;

    ESP_LOGI(TAG, "🔄 [MQTT-PUBLISHER] MQTT Publisher task started");
    esp_task_wdt_add(NULL); // Add task to WDT

    while (1) {
        esp_task_wdt_reset(); // Feed WDT
        // Esperar dato de la cola (Con timeout para alimentar WDT)
        if (xQueueReceive(ble_evt_queue, &msg, pdMS_TO_TICKS(1000)) == pdTRUE) {

            // Verificar conexión WiFi antes de intentar nada
            ESP_LOGI(TAG, "🔍 [MQTT-PUBLISHER] Checking WiFi status before publishing...");
            EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, pdMS_TO_TICKS(100));

            if (bits & WIFI_CONNECTED_BIT) {
                ESP_LOGI(TAG, "✅ [MQTT-PUBLISHER] WiFi connected - Proceeding with MQTT publish");
                // Crear JSON
                cJSON *root = cJSON_CreateObject();
                cJSON_AddStringToObject(root, "device", msg.device_name);
                cJSON_AddNumberToObject(root, "rssi", msg.rssi);
                
                char mac_str[18];
                snprintf(mac_str, sizeof(mac_str), MACSTR, MAC2STR(msg.addr));
                cJSON_AddStringToObject(root, "mac", mac_str);

                char uuid_str[37]; // 32 chars + 4 hyphens + null terminator
                sprintf(uuid_str, "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                        msg.uuid[0], msg.uuid[1], msg.uuid[2], msg.uuid[3],
                        msg.uuid[4], msg.uuid[5], msg.uuid[6], msg.uuid[7],
                        msg.uuid[8], msg.uuid[9], msg.uuid[10], msg.uuid[11],
                        msg.uuid[12], msg.uuid[13], msg.uuid[14], msg.uuid[15]);
                cJSON_AddStringToObject(root, "uuid", uuid_str);
                cJSON_AddNumberToObject(root, "major", msg.major);
                cJSON_AddNumberToObject(root, "minor", msg.minor);
                cJSON_AddNumberToObject(root, "tx_power", msg.tx_power);

                // Serializar
                char *json_str = cJSON_PrintUnformatted(root);
                
                // Publicar
                ESP_LOGI(TAG, "📤 [MQTT-PUBLISH] Publishing to topic: %s", MQTT_TOPIC_BEACON);
                int msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_BEACON, json_str, 0, 1, 0);
                if (msg_id != -1) {
                    ESP_LOGI(TAG, "✅ [MQTT-PUBLISH] Message queued with ID: %d", msg_id);
                    ESP_LOGI(TAG, "📝 [MQTT-PUBLISH] Payload: %s", json_str);
                } else {
                    ESP_LOGE(TAG, "❌ [MQTT-PUBLISH] Error al publicar MQTT - esp_mqtt_client_publish failed");
                }

                // Limpieza
                free(json_str); // cJSON_Print asigna memoria
                cJSON_Delete(root);
            } else {
                ESP_LOGW(TAG, "❌ [MQTT-PUBLISHER] WiFi no conectado. Descartando paquete BLE.");
            }
        }
    }
}

/* --- HEARTBEAT COUNTER TASK --- */
static void heartbeat_counter_task(void *pvParameters)
{
    ESP_LOGI(TAG, "💗 [HEARTBEAT] Heartbeat Counter Task iniciada. Intervalo: %d ms", COUNTER_INTERVAL_MS);

    esp_task_wdt_add(NULL); // STRICT: Add task to WDT

    uint32_t counter = 0;
    char counter_json[256];
    uint32_t accumulated_ms = 0;
    const uint32_t WAKE_INTERVAL_MS = 1000; // Wake up every 1s to feed dog

    for (;;) {
        // Feed the dog frequently
        esp_task_wdt_reset();
        
        vTaskDelay(pdMS_TO_TICKS(WAKE_INTERVAL_MS));
        accumulated_ms += WAKE_INTERVAL_MS;

        if (accumulated_ms >= COUNTER_INTERVAL_MS) {
            accumulated_ms = 0; // Reset timer

            // Check WiFi and MQTT status
            EventBits_t wifi_bits = xEventGroupGetBits(s_wifi_event_group);
            bool wifi_connected = (wifi_bits & WIFI_CONNECTED_BIT);

            ESP_LOGI(TAG, "🔍 [HEARTBEAT] Counter #%lu - WiFi: %s, MQTT Client: %s",
                     counter + 1,
                     wifi_connected ? "CONNECTED ✅" : "DISCONNECTED ❌",
                     (mqtt_client != NULL) ? "EXISTS ✅" : "NULL ❌");

            if (wifi_connected && mqtt_client != NULL) {
                counter++;

                // Crear JSON para el contador
                snprintf(counter_json, sizeof(counter_json),
                         "{"
                         "\"counter\": %lu,"
                         "\"timestamp\": %lld,"
                         "\"device_id\": \"%s\","
                         "\"status\": \"online\","
                         "\"wifi_ssid\": \"%s\","
                         "\"rssi\": 0"
                         "}",
                         counter,
                         esp_timer_get_time() / 1000000,
                         MQTT_USERNAME,
                         WIFI_SSID
                );

                ESP_LOGI(TAG, "📤 [HEARTBEAT] Publishing heartbeat #%lu to topic: %s", counter, MQTT_TOPIC_COUNTER);
                ESP_LOGI(TAG, "📝 [HEARTBEAT] Payload: %s", counter_json);

                int msg_id = esp_mqtt_client_publish(mqtt_client,
                                                   MQTT_TOPIC_COUNTER,
                                                   counter_json,
                                                   strlen(counter_json),
                                                   1,  // QoS 1
                                                   0); // Retain 0

                if (msg_id != -1) {
                    ESP_LOGI(TAG, "✅ [HEARTBEAT] Heartbeat #%lu QUEUED to %s (msg_id=%d)",
                            counter, MQTT_TOPIC_COUNTER, msg_id);
                } else {
                    ESP_LOGE(TAG, "❌ [HEARTBEAT] Error queueing heartbeat #%lu - esp_mqtt_client_publish failed", counter);
                }
            } else {
                ESP_LOGW(TAG, "⏸️ [HEARTBEAT] WiFi/MQTT no disponible, skipping heartbeat #%lu", counter + 1);
            }
        }
    }
}

void app_main(void)
{
    // 1. Inicialización NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // 1.1 Load Configuration from NVS
    load_config_from_nvs();

    // 2. Crear Cola de Eventos
    // Capacidad para 20 eventos (Aumentado para robustez)
    ble_evt_queue = xQueueCreate(20, sizeof(ble_msg_t));

    // 3. Iniciar WiFi y MQTT
    wifi_init_sta();
    mqtt_app_start();

    // 4. Iniciar NimBLE
    nimble_port_init();
    ble_hs_cfg.sync_cb = ble_app_on_sync;
    nimble_port_freertos_init(host_task);

    // 5. Iniciar Tarea Publicadora
    xTaskCreate(mqtt_publisher_task, "mqtt_pub", 4096, NULL, 5, NULL);

    // 6. Iniciar Tarea Heartbeat Counter
    xTaskCreate(heartbeat_counter_task, "heartbeat_counter", 4096, NULL, 4, NULL);
    ESP_LOGI(TAG, "Tarea Heartbeat Counter iniciada");
}
