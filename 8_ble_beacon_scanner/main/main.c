/*
 * SAFETY-CRITICAL BEACON SCANNER (iBeacon / Eddystone Aware)
 * Role: Principal Embedded Architect Implementation
 *
 * Features:
 * - Intelligent Parsing: Decodes iBeacon structure securely.
 * - Noise Filtering: RSSI thresholding.
 * - Health Monitoring: Watchdog warns if radio silence is detected.
 * - Stack Safety: No dynamic allocation for parsing.
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

static const char *TAG = "SAFE_SCANNER";

// Configuration
#define RSSI_THRESHOLD -85 // Ignore signals weaker than this
// NOTE: For testing at ~1m, -85 dBm is suitable.
// For production at ~10m, consider adjusting to -95 dBm or lower based on environmental testing.
// A lower (more negative) threshold increases sensitivity and detection range.
#define SCAN_WINDOW    100 // ms
#define SCAN_INTERVAL  200 // ms (50% Duty Cycle for power saving)

// Beacon Constants
#define IBEACON_MFG_ID 0x004C // Apple Inc.

// Structs for Parsing (Packed to match raw bytes)
typedef struct {
    uint16_t mfg_id;
    uint8_t  sub_type;
    uint8_t  length;
    uint8_t  proximity_uuid[16];
    uint16_t major;
    uint16_t minor;
    int8_t   tx_power;
} __attribute__((packed)) ibeacon_data_t;

// Health Monitor
static volatile uint32_t packets_received = 0;

// ============================================================================ 
// PARSING LOGIC
// ============================================================================ 

void print_ibeacon(const ibeacon_data_t *beacon, int8_t rssi, const ble_addr_t *addr) {
    // Convert UUID to string manually to avoid heap usage
    ESP_LOGI(TAG, "--- iBEACON DETECTED ---");
    ESP_LOGI(TAG, "Addr: %02x:%02x:%02x:%02x:%02x:%02x | RSSI: %d",
             addr->val[5], addr->val[4], addr->val[3],
             addr->val[2], addr->val[1], addr->val[0], rssi); 
    
    // UUID is usually big endian in iBeacon spec, but we receive raw bytes
    // We just print hex for verification
    ESP_LOG_BUFFER_HEX_LEVEL("UUID", beacon->proximity_uuid, 16, ESP_LOG_INFO);
    
    // Major/Minor are Big Endian in air, we need to swap for ESP32 (Little Endian)
    uint16_t major = __builtin_bswap16(beacon->major);
    uint16_t minor = __builtin_bswap16(beacon->minor); 
    
    ESP_LOGI(TAG, "Major: %u, Minor: %u, TxPwr: %d\n", major, minor, beacon->tx_power);
}

// ============================================================================ 
// GAP EVENT HANDLER
// ============================================================================ 

static int ble_gap_event(struct ble_gap_event *event, void *arg) {
    struct ble_hs_adv_fields fields;
    int rc;

    switch (event->type) {
        case BLE_GAP_EVENT_DISC:
            // 1. RSSI Filter (Early exit to save CPU)
            if (event->disc.rssi < RSSI_THRESHOLD) {
                return 0;
            }

            packets_received++; // Watchdog Feed Logic

            // 2. Parse Fields
            rc = ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data);
            if (rc != 0) return 0;

            // 3. Look for Manufacturer Data (iBeacon)
            if (fields.mfg_data != NULL && fields.mfg_data_len == 25) {
                // Check if it's an iBeacon (Apple ID 0x004C)
                // Need to be careful with casting raw bytes
                uint16_t mfg_id = fields.mfg_data[0] | (fields.mfg_data[1] << 8);
                
                if (mfg_id == IBEACON_MFG_ID) {
                    // Safety: Cast to struct only if length is validated
                    // The mfg_data points to the data AFTER the length byte in standard parsing, 
                    // but NimBLE returns the whole block usually. 
                    // iBeacon structure in mfg_data:
                    // [0-1] ID (4C 00)
                    // [2]   Type (02)
                    // [3]   Len (15)
                    // [4-19] UUID
                    // [20-21] Major
                    // [22-23] Minor
                    // [24]    Tx
                    
                    if (fields.mfg_data[2] == 0x02 && fields.mfg_data[3] == 0x15) {
                        // Secure Copy: Avoid pointer casting on potentially unaligned stack buffer
                        ibeacon_data_t beacon_data;
                        memcpy(&beacon_data, fields.mfg_data, sizeof(ibeacon_data_t));
                        
                        print_ibeacon(&beacon_data, event->disc.rssi, &event->disc.addr);
                    }
                }
            }
            
            // 4. Look for Eddystone (Service UUID 0xFEAA)
            // (Simplified check for demonstration)
            for (int i = 0; i < fields.num_uuids16; i++) {
                if (ble_uuid_u16(&fields.uuids16[i].u) == 0xFEAA) {
                    ESP_LOGI(TAG, "Eddystone Beacon Detected (RSSI: %d)", event->disc.rssi);
                }
            }
            
            return 0;

        default:
            return 0;
    }
}

static void ble_app_scan(void) {
    // Safety: Stack allocated parameters
    struct ble_gap_disc_params disc_params = {0}; 
    
    disc_params.filter_duplicates = 1; // Critical for avoiding log flood
    disc_params.passive = 1;           // Passive scanning (listen only)
    disc_params.itvl = SCAN_INTERVAL / 0.625; // Units of 0.625ms
    disc_params.window = SCAN_WINDOW / 0.625; 
    disc_params.filter_policy = BLE_HCI_SCAN_FILT_NO_WL;
    disc_params.limited = 0;

    int rc = ble_gap_disc(BLE_OWN_ADDR_PUBLIC, BLE_HS_FOREVER, &disc_params, ble_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error initiating scan: %d", rc);
    } else {
        ESP_LOGI(TAG, "Scanner started (RSSI Threshold: %d dBm)", RSSI_THRESHOLD);
    }
}

// ============================================================================ 
// SYSTEM
// ============================================================================ 

void ble_on_sync(void) {
    ble_hs_util_ensure_addr(0);
    ble_app_scan();
}

void host_task(void *param) {
    nimble_port_run();
    nimble_port_freertos_deinit();
}

// Watchdog Monitor Task
// Checks if we are receiving ANY packets. If silence > 30s, warn.
void monitor_task(void *param) {
    esp_task_wdt_add(NULL);
    uint32_t last_packets = 0;
    const int check_interval_ms = 10000;
    const int wake_interval_ms = 2000; // Wake often to feed the dog
    int elapsed_ms = 0;
    
    while(1) {
        esp_task_wdt_reset(); // Feed WDT every 2s
        vTaskDelay(pdMS_TO_TICKS(wake_interval_ms));
        elapsed_ms += wake_interval_ms;
        
        // Execute business logic every 10s
        if (elapsed_ms >= check_interval_ms) {
            elapsed_ms = 0;
            if (packets_received == last_packets) {
                ESP_LOGW(TAG, "WARNING: No BLE packets received in last 10s. Check Antenna/Environment.");
            } else {
                ESP_LOGI(TAG, "Status: Scanning active. Total packets: %lu", packets_received);
            }
            last_packets = packets_received;
        }
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "=== SAFETY CRITICAL BEACON SCANNER ===");
    
    nvs_flash_init();
    nimble_port_init();
    
    ble_hs_cfg.sync_cb = ble_on_sync;
    nimble_port_freertos_init(host_task);
    
    xTaskCreate(monitor_task, "monitor", 2048, NULL, 5, NULL);
}
