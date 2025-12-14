/*
 * CLIENTE MAESTRO - VERSIÓN "TURBO LINK"
 * Objetivo: Conexión rápida y lectura inmediata para evitar Timeouts.
 */

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"

static const char *TAG = "BLE_CLIENT";

// Tiempos
#define SCAN_DURATION_MS    10000
#define CONNECT_TIMEOUT_MS  10000

// UUIDs (Sincronizados Little-Endian)
static const ble_uuid128_t gatt_svr_svc_uuid = BLE_UUID128_INIT(
    0x00, 0x00, 0x00, 0x00, 0x4a, 0x01, 0x45, 0x93,
    0x94, 0x48, 0x18, 0x86, 0xde, 0x84, 0x50, 0x28
);

static const ble_uuid128_t gatt_svr_chr_uuid = BLE_UUID128_INIT(
    0x01, 0x00, 0x00, 0x00, 0x4a, 0x01, 0x45, 0x93,
    0x94, 0x48, 0x18, 0x86, 0xde, 0x84, 0x50, 0x28
);

static uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE;
static bool is_scanning = false;

// Forward Declarations
static void ble_app_scan(void);
static int ble_gap_event(struct ble_gap_event *event, void *arg);

// ============================================================================
// GATT (LECTURA)
// ============================================================================

static int ble_gattc_read_cb(uint16_t conn_handle, const struct ble_gatt_error *error,
                             struct ble_gatt_attr *attr, void *arg) {
    if (error->status == 0) {
        // ÉXITO: Imprimimos el dato recibido
        ESP_LOGI(TAG, ">>> ¡LECTURA EXITOSA! Data: %.*s", attr->om->om_len, attr->om->om_data);
        
        // (Opcional) Desconectar tras leer para probar ciclos
        // ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    } else {
        ESP_LOGE(TAG, "Error leyendo: status=%d (7=Auth, 13=Timeout)", error->status);
    }
    return 0;
}

static int ble_gattc_disc_chr_cb(uint16_t conn_handle, const struct ble_gatt_error *error,
                                 const struct ble_gatt_chr *chr, void *arg) {
    if (error->status == 0) {
        if (ble_uuid_cmp(&chr->uuid.u, &gatt_svr_chr_uuid.u) == 0) {
            ESP_LOGI(TAG, "¡Característica Target encontrada! Solicitando lectura...");
            
            // Leemos inmediatamente
            int rc = ble_gattc_read(conn_handle, chr->val_handle, ble_gattc_read_cb, NULL);
            if (rc != 0) ESP_LOGE(TAG, "Fallo al encolar lectura: %d", rc);
        }
    }
    return 0;
}

static int ble_gattc_disc_svc_cb(uint16_t conn_handle, const struct ble_gatt_error *error,
                                 const struct ble_gatt_svc *svc, void *arg) {
    if (error->status == 0) {
        if (ble_uuid_cmp(&svc->uuid.u, &gatt_svr_svc_uuid.u) == 0) {
            ESP_LOGI(TAG, "¡Servicio Target encontrado! Escaneando características...");
            ble_gattc_disc_all_chrs(conn_handle, svc->start_handle, svc->end_handle,
                                    ble_gattc_disc_chr_cb, NULL);
        }
    }
    return 0;
}

// ============================================================================
// GAP (CONEXIÓN OPTIMIZADA)
// ============================================================================

static void ble_app_connect(const ble_addr_t *addr) {
    struct ble_gap_conn_params conn_params = {0};
    
    // --- CONFIGURACIÓN DE ALTA VELOCIDAD ---
    // Forzamos intervalos cortos para que la negociación y lectura
    // ocurran antes de cualquier timeout.
    
    conn_params.scan_itvl = 0x0010; // 10ms
    conn_params.scan_window = 0x0010; // 10ms
    
    conn_params.itvl_min = 12;  // 12 * 1.25ms = 15ms (Muy rápido)
    conn_params.itvl_max = 24;  // 24 * 1.25ms = 30ms
    conn_params.latency = 0;    // Sin latencia, esclavo responde siempre
    conn_params.supervision_timeout = 400; // 400 * 10ms = 4 segundos (Suficiente)
    conn_params.min_ce_len = 0;
    conn_params.max_ce_len = 0;

    if (is_scanning) {
        ble_gap_disc_cancel();
        is_scanning = false;
    }

    ESP_LOGI(TAG, "Iniciando conexión con parámetros agresivos...");
    int rc = ble_gap_connect(BLE_OWN_ADDR_PUBLIC, addr, CONNECT_TIMEOUT_MS, 
                             &conn_params, ble_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error iniciando conexión: rc=%d", rc);
        ble_app_scan();
    }
}

static int ble_gap_event(struct ble_gap_event *event, void *arg) {
    struct ble_hs_adv_fields fields;
    int rc;

    switch (event->type) {
        case BLE_GAP_EVENT_DISC:
            rc = ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data);
            if (rc != 0) return 0;

            for (int i = 0; i < fields.num_uuids128; i++) {
                if (ble_uuid_cmp(&fields.uuids128[i].u, &gatt_svr_svc_uuid.u) == 0) {
                    ESP_LOGI(TAG, "¡OBJETIVO DETECTADO! Conectando...");
                    ble_app_connect(&event->disc.addr);
                    return 0;
                }
            }
            return 0;

        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                ESP_LOGI(TAG, "¡CONEXIÓN ESTABLECIDA! Handle=%d", event->connect.conn_handle);
                conn_handle = event->connect.conn_handle;
                ble_gattc_disc_all_svcs(conn_handle, ble_gattc_disc_svc_cb, NULL);
            } else {
                ESP_LOGE(TAG, "Fallo conexión: status=%d", event->connect.status);
                ble_app_scan();
            }
            return 0;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "Desconectado (Razón=%d). Reiniciando escaneo...", event->disconnect.reason);
            conn_handle = BLE_HS_CONN_HANDLE_NONE;
            ble_app_scan();
            return 0;

        default:
            return 0;
    }
}

static void ble_app_scan(void) {
    struct ble_gap_disc_params disc_params = {0};
    disc_params.filter_duplicates = 1;
    disc_params.passive = 0;
    disc_params.itvl = 0;
    disc_params.window = 0;

    ESP_LOGI(TAG, "Escaneando...");
    ble_gap_disc(BLE_OWN_ADDR_PUBLIC, SCAN_DURATION_MS, &disc_params, ble_gap_event, NULL);
    is_scanning = true;
}

// ============================================================================
// ARRANQUE
// ============================================================================

void ble_client_on_sync(void) {
    int rc = ble_hs_util_ensure_addr(0);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error crítico: No se pudo obtener dirección (rc=%d)", rc);
        return;
    }
    ESP_LOGI(TAG, "Sincronizado con BLE. Iniciando escaneo...");
    ble_app_scan();
}

void ble_host_task(void *param) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void app_main(void) {
    nvs_flash_init();
    nimble_port_init();
    ble_svc_gap_init();
    
    ble_hs_cfg.sync_cb = ble_client_on_sync;
    nimble_port_freertos_init(ble_host_task);
}