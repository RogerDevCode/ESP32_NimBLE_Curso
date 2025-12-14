# The Silent Observer: A High-Performance BLE Scanner

## Overview

In the vast, invisible ocean of radio waves associated with the Internet of Things, efficiency is not merely a feature—it is survival. This project, **The Silent Observer**, is a precision-engineered ESP32 application designed with one singular purpose: to scan, detect, and communicate with BLE peripherals with surgical speed and absolute reliability.

Leveraging the lightweight NimBLE stack and the robustness of ESP-IDF v5.5.1, this client operates asynchronously, rejecting the sluggish blocking delays of yesterday in favor of a reactive, event-driven architecture. It detects specific GATT services, connects with aggressive timing parameters to minimize latency, extracts data, and prepares it for upstream processing—all while respecting the finite energy resources of its host.

## Technical Architecture

*   **Core**: ESP32 / ESP32-S3
*   **Framework**: ESP-IDF v5.5.1 (FreeRTOS)
*   **Stack**: Apache NimBLE (Bluetooth 5.1 certified)
*   **Operations**:
    *   **Continuous Scanning**: Filters duplicates to reduce noise.
    *   **Turbo Link Connection**: Negotiates low-latency connection intervals (15ms-30ms) for rapid data exchange.
    *   **GATT Client**: Automates service discovery and characteristic reading.

## Getting Started

### Prerequisites

*   **Hardware**: An ESP32 development board.
*   **Software**:
    *   VS Code with Espressif IDF Extension.
    *   ESP-IDF v5.5.1 installed and configured.
    *   Python environment active (alias `idfon`).

### Installation & Configuration

1.  **Clone the Repository**:
    Ensure you are in your project workspace.

2.  **Configuration**:
    The project relies on `sdkconfig.defaults` to pre-load critical NimBLE settings.
    ```bash
    idf.py set-target esp32
    idf.py menuconfig
    ```
    *Verify that "Component config > Bluetooth > NimBLE Options" is enabled.*

3.  **Build**:
    Compile the firmware with precision.
    ```bash
    idf.py build
    ```

4.  **Flash & Monitor**:
    Inject the soul into the silicon and watch it breathe.
    ```bash
    idf.py -p /dev/ttyUSB0 flash monitor
    ```
    *(Replace `/dev/ttyUSB0` with your actual serial port).*

## Operational Logic

The system follows a strict state machine:
1.  **Scan**: It listens for advertisements containing the target Service UUID.
2.  **Connect**: Upon positive ID, it initiates a connection with "aggressive" parameters to minimize handshake time.
3.  **Discovery**: It traverses the GATT table to find the target Characteristic.
4.  **Action**: It reads the data immediately.
5.  **Recovery**: If the link is severed, it does not mourn; it simply returns to scanning, ready for the next cycle.

## Edge Cases & Handling

*   **Connection Timeout**: If the peripheral is shy or distant, the system aborts the attempt after 10 seconds and resumes scanning.
*   **Packet Collision**: In high-density BLE environments, the duplicate filter ensures we process unique advertisements only once per scan window.
*   **Memory constraints**: All BLE events are handled in the NimBLE task context to prevent stack overflows in the main application loop.

---
*"Precision is the difference between a butcher and a surgeon."*
