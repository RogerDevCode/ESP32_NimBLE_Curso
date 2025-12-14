/*
 * SAFETY-CRITICAL BLE SERVER (NOTIFY ONLY)
 * Role: Principal Embedded Architect Implementation
 *
 * Features:
 * - Periodic Notifications (Telemetry Pattern).
 * - Safety: Memory aligned UUIDs, Stack-based Advertising config.
 * - Stability: Watchdog protected task.
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
#include "services/gatt/ble_svc_gatt.h"
#include "esp_task_wdt.h"

static const char *TAG = "SAFE_NOTIFY";
#define DEVICE_NAME "ESP32_NOTIFIER"

// Global Handle
static uint16_t g_conn_handle = BLE_HS_CONN_HANDLE_NONE;
static uint16_t val_handle;
static uint32_t counter = 0;

// UUIDs (Little Endian)
// Service: 00000000-0000-0000-0000-0000000000AA (Example)
// Char:    00000000-0000-0000-0000-0000000000BB
static const ble_uuid128_t gatt_svc_uuid = BLE_UUID128_INIT(
    0xAA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
);

static const ble_uuid128_t gatt_chr_uuid = BLE_UUID128_INIT(
    0xBB, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
);

static int gatt_svr_access(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt, void *arg);

// GATT Definition
static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &gatt_chr_uuid.u,
                .access_cb = gatt_svr_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &val_handle,
            },
            { 0 }
        },
    },
    { 0 }
};

// ============================================================================
// NOTIFICATION TASK
// ============================================================================
void notify_task(void *pvParameters) {
    esp_task_wdt_add(NULL);
    
    while(1) {
        esp_task_wdt_reset();
        
        if (g_conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            counter++;
            struct os_mbuf *om = ble_hs_mbuf_from_flat(&counter, sizeof(counter));
            if (om) {
                int rc = ble_gatts_notify_custom(g_conn_handle, val_handle, om);
                if (rc == 0) {
                    ESP_LOGI(TAG, "Notification Sent: %lu", counter);
                }
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static int gatt_svr_access(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_access_ctxt *ctxt, void *arg) {
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        os_mbuf_append(ctxt->om, &counter, sizeof(counter));
        return 0;
    }
    return BLE_ATT_ERR_UNLIKELY;
}

// ============================================================================
// GAP
// ============================================================================
static void start_advertising(void);

static int ble_gap_event(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            ESP_LOGI(TAG, "Connected");
            g_conn_handle = event->connect.conn_handle;
            break;
        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "Disconnected");
            g_conn_handle = BLE_HS_CONN_HANDLE_NONE;
            start_advertising();
            break;
    }
    return 0;
}

static void start_advertising(void) {
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields = {0};
    struct ble_hs_adv_fields rsp_fields = {0};
    int rc;

    // Safety: Stack Allocation for UUIDs
    ble_uuid128_t uuids[1];
    memcpy(&uuids[0], &gatt_svc_uuid, sizeof(gatt_svc_uuid));

    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.uuids128 = uuids;
    fields.num_uuids128 = 1;
    fields.uuids128_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) ESP_LOGE(TAG, "ADV Error: %d", rc);

    rsp_fields.name = (uint8_t *)DEVICE_NAME;
    rsp_fields.name_len = strlen(DEVICE_NAME);
    rsp_fields.name_is_complete = 1;
    
    rc = ble_gap_adv_rsp_set_fields(&rsp_fields);
    if (rc != 0) ESP_LOGE(TAG, "RSP Error: %d", rc);

    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                           &adv_params, ble_gap_event, NULL);
    if (rc != 0) ESP_LOGE(TAG, "Start ADV Error: %d", rc);
}

void ble_on_sync(void) {
    ble_hs_util_ensure_addr(0);
    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_gatts_count_cfg(gatt_svcs);
    ble_gatts_add_svcs(gatt_svcs);
    ble_gatts_start();
    start_advertising();
}

void host_task(void *param) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void app_main(void) {
    nvs_flash_init();
    nimble_port_init();
    ble_hs_cfg.sync_cb = ble_on_sync;
    nimble_port_freertos_init(host_task);
    
    xTaskCreate(notify_task, "notify", 2048, NULL, 5, NULL);
}