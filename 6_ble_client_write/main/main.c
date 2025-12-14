/*
 * SAFETY-CRITICAL BLE CLIENT (ACTUATOR CONTROLLER)
 * Role: Principal Embedded Architect Implementation
 * 
 * Features:
 * - CLOSED-LOOP CONTROL: Writes -> Waits for Verification Notification.
 * - RETRY LOGIC: Persistent retries until success (Mission Critical).
 * - STATE MACHINE: Explicit handling of connection, subscription, and writing.
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
#include "esp_task_wdt.h"
#include "freertos/semphr.h"

static const char *TAG = "SAFE_CLIENT_CTRL";

// UUIDs (Must match Server)
static const ble_uuid128_t gatt_svc_uuid = BLE_UUID128_INIT(
    0x00, 0x00, 0x00, 0x00, 0x4a, 0x01, 0x45, 0x93,
    0x94, 0x48, 0x18, 0x86, 0xde, 0x84, 0x50, 0x28
);

static const ble_uuid128_t gatt_chr_uuid = BLE_UUID128_INIT(
    0x01, 0x00, 0x00, 0x00, 0x4a, 0x01, 0x45, 0x93,
    0x94, 0x48, 0x18, 0x86, 0xde, 0x84, 0x50, 0x28
);

// Control Variables
static uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE;
static uint16_t attr_handle = 0; // The characteristic value handle
static bool is_connected = false;
static bool is_subscribed = false;

// Synchronization for Closed-Loop
static SemaphoreHandle_t feedback_sem;
static uint8_t pending_val_to_verify = 0xFF; // 0xFF = None

// State Machine for the Controller Task
static uint8_t target_state = 0; // We want to toggle this (0 -> 1 -> 0)

// Forward Declarations
static void ble_app_scan(void);

// ============================================================================
// GATT CALLBACKS (Discovery & Notification)
// ============================================================================

static int ble_gattc_disc_chr_cb(uint16_t conn_handle, const struct ble_gatt_error *error,
                                 const struct ble_gatt_chr *chr, void *arg) {
    if (error->status == 0) {
        if (ble_uuid_cmp(&chr->uuid.u, &gatt_chr_uuid.u) == 0) {
            ESP_LOGI(TAG, "Actuator Char Found (Handle: %d)", chr->val_handle);
            attr_handle = chr->val_handle;

            // Subscribe to Notifications (Essential for Closed Loop)
            uint16_t cccd_val = BLE_GATT_CHR_PROP_NOTIFY;
            ble_gattc_write_flat(conn_handle, chr->val_handle + 1, 
                                 &cccd_val, sizeof(cccd_val), NULL, NULL);
            is_subscribed = true;
        }
    }
    return 0;
}

static int ble_gattc_disc_svc_cb(uint16_t conn_handle, const struct ble_gatt_error *error,
                                 const struct ble_gatt_svc *svc, void *arg) {
    if (error->status == 0) {
        if (ble_uuid_cmp(&svc->uuid.u, &gatt_svc_uuid.u) == 0) {
            ESP_LOGI(TAG, "Actuator Service Found");
            ble_gattc_disc_all_chrs(conn_handle, svc->start_handle, svc->end_handle,
                                    ble_gattc_disc_chr_cb, NULL);
        }
    }
    return 0;
}

// ============================================================================
// GAP EVENTS
// ============================================================================

static int ble_gap_event(struct ble_gap_event *event, void *arg) {
    struct ble_hs_adv_fields fields;
    int rc;

    switch (event->type) {
        case BLE_GAP_EVENT_DISC:
            rc = ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data);
            if (rc != 0) return 0;

            for (int i = 0; i < fields.num_uuids128; i++) {
                if (ble_uuid_cmp(&fields.uuids128[i].u, &gatt_svc_uuid.u) == 0) {
                    ESP_LOGI(TAG, "Target Found. Connecting...");
                    ble_gap_disc_cancel();
                    ble_gap_connect(BLE_OWN_ADDR_PUBLIC, &event->disc.addr, 
                                    5000, NULL, ble_gap_event, NULL);
                }
            }
            return 0;

        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0) {
                ESP_LOGI(TAG, "Connected");
                conn_handle = event->connect.conn_handle;
                is_connected = true;
                ble_gattc_disc_all_svcs(conn_handle, ble_gattc_disc_svc_cb, NULL);
            } else {
                ESP_LOGE(TAG, "Connection Failed");
                ble_app_scan();
            }
            return 0;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGW(TAG, "Disconnected. Retrying...");
            conn_handle = BLE_HS_CONN_HANDLE_NONE;
            is_connected = false;
            is_subscribed = false;
            ble_app_scan();
            return 0;

        case BLE_GAP_EVENT_NOTIFY_RX:
            ESP_LOGI(TAG, "RX NOTIFICATION (Handle %d)", event->notify_rx.attr_handle);
            if (event->notify_rx.attr_handle == attr_handle) {
                 uint8_t received_val = event->notify_rx.om->om_data[0];
                 ESP_LOGI(TAG, "VERIFICATION RECEIVED: Device is now %s", received_val ? "ON" : "OFF");
        
                 // Critical Check: Does it match what we wrote?
                 if (received_val == pending_val_to_verify) {
                     xSemaphoreGive(feedback_sem); // Signal success to control loop
                 } else {
                     ESP_LOGE(TAG, "MISMATCH! Wrote %d but got %d", pending_val_to_verify, received_val);
                 }
            }
            return 0;
    }
    return 0;
}

static void ble_app_scan(void) {
    struct ble_gap_disc_params disc_params = {0};
    disc_params.filter_duplicates = 1;
    disc_params.passive = 0;
    ble_gap_disc(BLE_OWN_ADDR_PUBLIC, BLE_HS_FOREVER, &disc_params, ble_gap_event, NULL);
}

// ============================================================================
// CONTROL TASK (THE BRAIN)
// ============================================================================
void control_task(void *param) {
    esp_task_wdt_add(NULL);
    
    ESP_LOGI(TAG, "Control Task Started. Waiting for Link...");

    while (1) {
        esp_task_wdt_reset();

        // 1. Wait for Valid Link
        if (!is_connected || !is_subscribed || attr_handle == 0) {
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        // 2. Determine Action (Toggle State)
        target_state = !target_state; // Toggle 0 <-> 1
        pending_val_to_verify = target_state;

        // 3. RETRY LOOP (Persistence)
        bool confirmed = false;
        while (!confirmed) {
            esp_task_wdt_reset();
            
            if (!is_connected) break; // Abort retry if link dies

            ESP_LOGI(TAG, ">>> COMMAND: Writing %d...", target_state);
            
            // Clear semaphore before writing
            xSemaphoreTake(feedback_sem, 0);

            // WRITE WITH RESPONSE (Layer 2 Ack)
            int rc = ble_gattc_write_flat(conn_handle, attr_handle, 
                                          &target_state, sizeof(target_state), 
                                          NULL, NULL);
            
            if (rc == 0) {
                // Wait for Layer 7 Ack (Notification)
                // Timeout: 2000ms
                if (xSemaphoreTake(feedback_sem, pdMS_TO_TICKS(2000)) == pdTRUE) {
                    ESP_LOGI(TAG, "<<< SUCCESS: State %d Confirmed.", target_state);
                    confirmed = true;
                } else {
                    ESP_LOGW(TAG, "TIMEOUT: No verification received. Retrying...");
                }
            } else {
                ESP_LOGE(TAG, "Write Failed (rc=%d). Retrying...", rc);
                vTaskDelay(pdMS_TO_TICKS(500)); // Short backoff
            }
        }

        // 4. Mission Accomplished (Wait before next toggle)
        if (confirmed) {
            ESP_LOGI(TAG, "Task Complete. Sleeping 5s...");
            vTaskDelay(pdMS_TO_TICKS(5000));
        }
    }
}

// ============================================================================
// MAIN
// ============================================================================
void ble_on_sync(void) {
    ble_hs_util_ensure_addr(0);
    ble_app_scan();
}

void host_task(void *param) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void app_main(void) {
    // Semaphore for Closed-Loop Sync
    feedback_sem = xSemaphoreCreateBinary();

    nvs_flash_init();
    nimble_port_init();
    ble_svc_gap_init();
    ble_hs_cfg.sync_cb = ble_on_sync;
    nimble_port_freertos_init(host_task);

    xTaskCreate(control_task, "controller", 4096, NULL, 5, NULL);
}
