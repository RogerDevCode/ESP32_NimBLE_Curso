/*
 * -----------------------------------------------------------------------------
 * Proyecto: 2 - Servidor BLE Básico (Advertising)
 * Autor: Roger Gallegos (adaptado por IA)
 * Fecha: 08-12-2025
 *
 * Objetivo:
 *    Configurar un ESP32 para que actúe como un servidor BLE básico.
 *    El dispositivo se anunciará (advertising) con un nombre específico ("ESP32-NimBLE")
 *    para que otros dispositivos BLE, como un smartphone, puedan descubrirlo.
 *
 * Componentes Clave de NimBLE Utilizados:
 *  - #include "host/ble_hs.h": Cabecera principal del Host de NimBLE.
 *  - #include "nimble/nimble_port.h": Funciones para inicializar el stack.
 *  - ble_hs_cfg: Estructura de configuración global del Host.
 *  - ble_svc_gap_device_name_set(): Establece el nombre del dispositivo BLE.
 *  - ble_gap_adv_start(): Inicia el proceso de advertising.
 *  - nimble_port_freertos_init(): Inicia la tarea del Host de NimBLE en FreeRTOS.
 *
 * Lógica Principal:
 *  1. `app_main` es el punto de entrada.
 *  2. Se inicializa NVS (Non-Volatile Storage), un requisito para almacenar información de BLE.
 *  3. Se inicializa el puerto de NimBLE, que crea una tarea de FreeRTOS para manejar el stack BLE.
 *  4. Se define una función `ble_app_on_sync` que se ejecutará cuando el stack esté listo.
 *  5. Dentro de `ble_app_on_sync`, se configura el nombre del dispositivo y los parámetros de advertising.
 *  6. Se llama a `ble_gap_adv_start` para comenzar a anunciarse.
 * -----------------------------------------------------------------------------
*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "host/ble_hs.h"
#include "host/util/util.h" // Added for ble_hs_util_ensure_addr
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "services/gap/ble_svc_gap.h"
#include "host/ble_gap.h" // Added for ble_gap_adv_rsp_set_data
#include "esp_task_wdt.h"


// Tag para los mensajes de log.
static const char *TAG = "BLE_ADV";

// Nombre del dispositivo BLE
static const char *device_name = "ESP32-NimBLE";

// iBeacon Constants
#define IBEACON_MFG_ID      0x004C
#define IBEACON_MAJOR       1010
#define IBEACON_MINOR       2020
#define IBEACON_UUID_ARR    {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11, \
                             0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99}

// --- Declaración de Funciones ---
void ble_app_on_sync(void);
void start_advertising(void);
void ble_host_task(void *param);
void watchdog_task(void *param);

void start_advertising(void) {
    int rc;
    
    // --- 1. Advertising Data (iBeacon) ---
    struct ble_hs_adv_fields fields = {0};
    
    uint8_t mfg_data[25];
    mfg_data[0] = IBEACON_MFG_ID & 0xFF;
    mfg_data[1] = (IBEACON_MFG_ID >> 8) & 0xFF;
    mfg_data[2] = 0x02;
    mfg_data[3] = 0x15;
    uint8_t uuid[16] = IBEACON_UUID_ARR;
    memcpy(&mfg_data[4], uuid, 16);
    mfg_data[20] = (IBEACON_MAJOR >> 8) & 0xFF;
    mfg_data[21] = IBEACON_MAJOR & 0xFF;
    mfg_data[22] = (IBEACON_MINOR >> 8) & 0xFF;
    mfg_data[23] = IBEACON_MINOR & 0xFF;
    mfg_data[24] = 0xC5;
    
    fields.mfg_data = mfg_data;
    fields.mfg_data_len = sizeof(mfg_data);
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to set adv fields: %d", rc);
        return;
    }

    // --- 2. Scan Response Data (Device Name) ---
    // Construct raw packet: [Len][Type][Data]...
    uint8_t scan_rsp_data[31];
    int scan_rsp_len = 0;
    int name_len = strlen(device_name);

    // AD Structure: Complete Local Name
    if (name_len > 0 && (scan_rsp_len + 2 + name_len) <= 31) {
        scan_rsp_data[scan_rsp_len++] = name_len + 1; // Length (Type + Name)
        scan_rsp_data[scan_rsp_len++] = BLE_HS_ADV_TYPE_COMP_NAME; // 0x09
        memcpy(&scan_rsp_data[scan_rsp_len], device_name, name_len);
        scan_rsp_len += name_len;
    }

    rc = ble_gap_adv_rsp_set_data(scan_rsp_data, scan_rsp_len);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to set scan rsp data: %d", rc);
        return;
    }

    // --- 3. Start Advertising ---
    struct ble_gap_adv_params adv_params = {0};
    adv_params.conn_mode = BLE_GAP_CONN_MODE_NON;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    adv_params.itvl_min = 160;
    adv_params.itvl_max = 160; 

    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &adv_params, NULL, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to start advertising: %d", rc);
        return;
    }
    
    ESP_LOGI(TAG, "iBeacon + Name Broadcasting Started");
}

/**
 * @brief Función de callback que se ejecuta cuando el stack BLE está sincronizado y listo.
 */
void ble_app_on_sync(void)
{
    ESP_LOGI(TAG, "BLE Host sincronizado.");
    ble_hs_util_ensure_addr(0); // Auto-generate address if needed
    start_advertising();
}

/**
 * @brief Tarea del host de NimBLE.
 */
void ble_host_task(void *param)
{
    ESP_LOGI(TAG, "Tarea del Host BLE iniciada.");
    // Esta función ejecuta el loop de procesamiento de eventos de NimBLE.
    // No retorna hasta que nimble_port_stop() es llamado.
    nimble_port_run();

    nimble_port_freertos_deinit();
}

// Watchdog Monitor Task
void watchdog_task(void *param) {
    ESP_LOGI(TAG, "Watchdog Task Started");
    esp_task_wdt_add(NULL);
    
    while(1) {
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}


// --- Punto de Entrada Principal ---
void app_main(void)
{
    int rc;

    // Inicializa NVS (Non-Volatile Storage).
    // Es requerido por el stack BLE para almacenar claves y otra información.
    rc = nvs_flash_init();
    if (rc == ESP_ERR_NVS_NO_FREE_PAGES || rc == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        rc = nvs_flash_init();
    }
    ESP_ERROR_CHECK(rc);

    ESP_LOGI(TAG, "Inicializando NimBLE Port.");
    // Inicializa el puerto de NimBLE. Esto debe ser lo primero.
    nimble_port_init();

    // Configura el callback de sincronización.
    ble_hs_cfg.sync_cb = ble_app_on_sync;
    
    // --- Código Crítico: Establecer el nombre del dispositivo ---
    // Este nombre será visible para otros dispositivos BLE.
    // Already set in adv_fields, so removing redundant call here.
    // rc = ble_svc_gap_device_name_set(device_name);
    // if (rc != 0) {
    //     ESP_LOGE(TAG, "Error al establecer el nombre del dispositivo; rc=%d", rc);
    // }

    // Inicia la tarea del host de NimBLE.
    nimble_port_freertos_init(ble_host_task);

    // Iniciar tarea de Watchdog
    xTaskCreate(watchdog_task, "wdt_feeder", 2048, NULL, 5, NULL);

    ESP_LOGI(TAG, "app_main finalizado. La lógica BLE corre en su propia tarea.");
}