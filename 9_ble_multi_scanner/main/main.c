/*
 * -----------------------------------------------------------------------------
 * Proyecto: 9 - Escáner Multi-Dispositivo Avanzado
 * Autor: Roger Gallegos (adaptado por IA)
 * Fecha: 08-12-2025
 *
 * Objetivo:
 *    Implementar un escáner capaz de rastrear múltiples dispositivos simultáneamente.
 * -----------------------------------------------------------------------------
*/

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "host/ble_hs.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "esp_task_wdt.h"
#include "host/util/util.h"

static const char *TAG = "MULTI_SCANNER";

// --- Estructuras de Datos ---
#define MAX_DEVICES 20 

typedef struct scanned_device {
    ble_addr_t addr;
    int8_t rssi;
    uint32_t count;
    char name[32]; 
    struct scanned_device *next;
} scanned_device_t;

typedef struct device_pool {
    scanned_device_t devices[MAX_DEVICES];
    bool used[MAX_DEVICES];
    int allocated_count;
} device_pool_t;

static scanned_device_t *device_list_head = NULL;
static int device_count = 0;
static SemaphoreHandle_t list_mutex = NULL;
static device_pool_t device_pool = {0};

// --- Declaraciones ---
void ble_app_scan(void);
static int ble_app_gap_event(struct ble_gap_event *event, void *arg);

// --- Gestión de Pool de Memoria Estática ---
static void init_device_pool(void) {
    for (int i = 0; i < MAX_DEVICES; i++) {
        device_pool.used[i] = false;
        memset(&device_pool.devices[i], 0, sizeof(scanned_device_t));
    }
    device_pool.allocated_count = 0;
}

static scanned_device_t* allocate_device(void) {
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (!device_pool.used[i]) {
            device_pool.used[i] = true;
            device_pool.allocated_count++;
            return &device_pool.devices[i];
        }
    }
    return NULL;
}

// --- Manejo Robusto de NVS ---
static esp_err_t init_nvs(void) {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    return ret;
}

// --- Gestión de la Lista de Dispositivos ---
static void safe_copy_name(char *dest, const uint8_t *src, uint8_t src_len) {
    if (dest == NULL) return;
    dest[0] = '\0';
    if (src == NULL || src_len == 0) return;

    // Use standard C sizeof idiom
    size_t max_len = sizeof(((scanned_device_t*)0)->name) - 1;
    int copy_len = (src_len < max_len) ? src_len : max_len;
    
    memcpy(dest, src, copy_len);
    dest[copy_len] = '\0';
}

static scanned_device_t* find_device(const ble_addr_t *addr) {
    scanned_device_t *current = device_list_head;
    while (current != NULL) {
        if (ble_addr_cmp(&current->addr, addr) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

static void update_device(const ble_addr_t *addr, int8_t rssi, const uint8_t *name, uint8_t name_len) {
    if (xSemaphoreTake(list_mutex, portMAX_DELAY) == pdTRUE) {
        scanned_device_t *dev = find_device(addr);

        if (dev) {
            dev->rssi = rssi;
            dev->count++;
            if (dev->name[0] == '\0' && name != NULL && name_len > 0) {
                safe_copy_name(dev->name, name, name_len);
            }
        } else {
            if (device_count < MAX_DEVICES) {
                scanned_device_t *new_dev = allocate_device();
                if (new_dev) {
                    new_dev->addr = *addr;
                    new_dev->rssi = rssi;
                    new_dev->count = 1;
                    new_dev->next = device_list_head;
                    safe_copy_name(new_dev->name, name, name_len);
                    device_list_head = new_dev;
                    device_count++;
                }
            }
        }
        xSemaphoreGive(list_mutex);
    }
}

// Tarea para imprimir el reporte periódico (WDT Safe)
void report_task(void *pvParameters) {
    esp_task_wdt_add(NULL);
    while(1) {
        for(int i=0; i<5; i++) {
            esp_task_wdt_reset();
            vTaskDelay(pdMS_TO_TICKS(1000));
        }

        if (xSemaphoreTake(list_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
            ESP_LOGI(TAG, "--- Reporte de Dispositivos (%d encontrados, %d en pool) ---",
                     device_count, device_pool.allocated_count);
            scanned_device_t *current = device_list_head;
            int i = 1;
            while (current != NULL) {
                ESP_LOGI(TAG, "[%2d] Addr: %02x:%02x:%02x:%02x:%02x:%02x | RSSI: %4d | Count: %4d | Name: %s",
                         i++,
                         current->addr.val[5], current->addr.val[4], current->addr.val[3],
                         current->addr.val[2], current->addr.val[1], current->addr.val[0],
                         current->rssi, current->count,
                         (strlen(current->name) > 0) ? current->name : "(Unknown)");
                current = current->next;
            }
            ESP_LOGI(TAG, "-----------------------------------------------");
            xSemaphoreGive(list_mutex);
        }
    }
}

// --- Callbacks BLE ---
static int ble_app_gap_event(struct ble_gap_event *event, void *arg) {
    struct ble_hs_adv_fields fields;
    int rc;

    switch (event->type) {
        case BLE_GAP_EVENT_DISC:
            rc = ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data);
            if (rc == 0) {
                // Check if name is present in the parsed fields
                if (fields.name != NULL && fields.name_len > 0) {
                    update_device(&event->disc.addr, event->disc.rssi, fields.name, fields.name_len);
                } else {
                    // Update device even if name is missing (it might have been set previously)
                    update_device(&event->disc.addr, event->disc.rssi, NULL, 0);
                }
            }
            return 0;
        default:
            return 0;
    }
}

void ble_app_scan(void) {
    struct ble_gap_disc_params disc_params;
    memset(&disc_params, 0, sizeof(disc_params));

    disc_params.filter_duplicates = 0;      
    disc_params.passive = 0;                // Active scanning to get names!
    disc_params.itvl = 30;                  
    disc_params.window = 30;                
    disc_params.filter_policy = 0;           
    disc_params.limited = 0;                 

    int rc = ble_gap_disc(BLE_OWN_ADDR_PUBLIC, BLE_HS_FOREVER, &disc_params, ble_app_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error iniciando escaneo; rc=%d", rc);
    } else {
        ESP_LOGI(TAG, "Escaneo multi-dispositivo iniciado (modo activo optimizado)");
    }
}

void ble_app_on_sync(void) {
    ESP_LOGI(TAG, "BLE Host sincronizado.");
    ble_hs_util_ensure_addr(0); 
    ble_app_scan();
}

void ble_host_task(void *param) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void app_main(void) {
    ESP_LOGI(TAG, "Iniciando Escáner BLE Multi-Dispositivo v2.0");

    esp_err_t ret = init_nvs();
    if (ret != ESP_OK) return;

    init_device_pool();

    list_mutex = xSemaphoreCreateMutex();
    if (list_mutex == NULL) return;

    ret = nimble_port_init();
    if (ret != ESP_OK) return;

    ble_hs_cfg.sync_cb = ble_app_on_sync;
    nimble_port_freertos_init(ble_host_task);

    xTaskCreate(report_task, "report_task", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "Sistema BLE Multi-Scanner inicializado exitosamente");
}
