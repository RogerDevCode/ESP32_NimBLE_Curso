# 10_ble_mqtt_gateway - Industrial BLE to MQTT Gateway

## 1. Purpose
This project implements a robust **BLE Scanner to MQTT Gateway** designed for industrial IoT applications. It scans for **iBeacon** devices, parses their Manufacturer Specific Data (UUID, Major, Minor, TxPower), and publishes this telemetry to an MQTT broker via Wi-Fi.

## 2. Architecture & Safety Features
*   **Layered Architecture:** Decouples BLE scanning (Producer) from MQTT publishing (Consumer) using a FreeRTOS Queue (`ble_evt_queue`).
*   **Watchdog Timer (WDT):** The user task `mqtt_publisher_task` is protected by `esp_task_wdt`.
*   **Memory Safety:** Static allocation for critical buffers; controlled queue size to prevent heap exhaustion.
*   **Filtering:** Implements strict iBeacon pattern matching (Apple ID 0x004C) to reduce noise.

## 3. Configuration
Configure via `idf.py menuconfig`:
*   `CONFIG_GATEWAY_WIFI_SSID`: Wi-Fi SSID.
*   `CONFIG_GATEWAY_WIFI_PASS`: Wi-Fi Password.
*   `CONFIG_GATEWAY_MQTT_URL`: Broker URL (e.g., `mqtt://broker.hivemq.com`).

## 4. Hardware Requirements
*   ESP32 Development Board (ESP-IDF v5.x).
*   Any standard iBeacon transmitter (or Project 2 configured as Beacon).

## 5. Output Format (JSON)
Topic: `esp32/gateway/data`
```json
{
  "device": "ESP32-iBeacon",
  "rssi": -85,
  "mac": "88:13:bf:23:a5:ae",
  "uuid": "aabbccdd-eeff-0011-2233-445566778899",
  "major": 1010,
  "minor": 2020,
  "tx_power": -59
}
```

## 6. Validation
*   **WDT:** Verified by monitoring `mqtt_publisher_task`.
*   **Leak Check:** Zero-copy where possible; careful cJSON handling.
*   **Connectivity:** Robust reconnection logic for Wi-Fi and MQTT.
