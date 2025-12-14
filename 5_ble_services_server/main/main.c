/*
 * SAFETY-CRITICAL IBEACON EMITTER (For Testing Scanner)
 * Role: Beacon Generator
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

static const char *TAG = "SAFE_IBEACON";

// iBeacon Configuration
static const uint8_t uuid128[16] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
};
static const uint16_t major = 10;
static const uint16_t minor = 20;
static const int8_t measured_power = -59; // RSSI at 1m

static void start_advertising(void) {
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields = {0};
    int rc;

    // Construct iBeacon Payload manually in Manufacturer Data
    // Format: [ID L][ID H][Type][Len][UUID...][Maj H][Maj L][Min H][Min L][Tx]
    uint8_t mfg_data[25];
    mfg_data[0] = 0x4C; // Apple ID Low
    mfg_data[1] = 0x00; // Apple ID High
    mfg_data[2] = 0x02; // Type: iBeacon
    mfg_data[3] = 0x15; // Length: 21 bytes remaining
    
    memcpy(&mfg_data[4], uuid128, 16);
    
    // Big Endian for Major/Minor in Air
    mfg_data[20] = (major >> 8) & 0xFF;
    mfg_data[21] = major & 0xFF;
    mfg_data[22] = (minor >> 8) & 0xFF;
    mfg_data[23] = minor & 0xFF;
    
    mfg_data[24] = measured_power;

    fields.mfg_data = mfg_data;
    fields.mfg_data_len = sizeof(mfg_data);
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "ADV Fields Error: %d", rc);
        return;
    }

    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_NON; // Non-connectable Beacon
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER,
                           &adv_params, NULL, NULL);
    if (rc != 0) ESP_LOGE(TAG, "ADV Start Error: %d", rc);
    else ESP_LOGI(TAG, "iBeacon Started!");
}

void ble_on_sync(void) {
    ble_hs_util_ensure_addr(0);
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
    
    esp_task_wdt_add(NULL);
    while(1) {
        esp_task_wdt_reset();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
