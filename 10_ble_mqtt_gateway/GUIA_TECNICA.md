# GUÍA TÉCNICA: Gateway BLE-MQTT Industrial

## 1. Introducción: El Puente IIoT
Este es el "Jefe Final". Un Gateway que conecta el mundo físico (Bluetooth) con la nube (MQTT/TLS).
*   **Entrada:** Paquetes BLE iBeacon.
*   **Procesamiento:** Filtrado, JSON packaging.
*   **Salida:** Publicación MQTT segura a HiveMQ Cloud.

Este dispositivo permite ver en un Dashboard web (Grafana/Node-RED) lo que ocurre físicamente en la planta.

---

## 2. Análisis del Código Crítico

### A. Seguridad TLS (Certificados)
No enviamos datos en texto plano. Usamos SSL/TLS puerto 8883.
```c
// Certificado raíz de HiveMQ embebido en el firmware
extern const uint8_t hivemq_cloud_cert_pem_start[] asm("_binary_hivemq_cloud_cert_pem_start");

.verification = {
    .crt_bundle_attach = esp_crt_bundle_attach, // Validación automática
}
```
Esto asegura que nadie en medio pueda leer o falsificar tus datos.

### B. Desacople (Producer-Consumer Pattern)
El callback BLE es rápido (interrupción). Enviar por WiFi/MQTT es lento (milisegundos o segundos). No podemos bloquear BLE esperando a WiFi.
**Solución:** Una cola FreeRTOS (`ble_evt_queue`).
1.  **BLE (Productor):** Mete mensaje en cola (µs).
2.  **MQTT Task (Consumidor):** Saca mensaje, formatea JSON, encripta y envía (ms).

### C. Configuración Dinámica (NVS)
Nada de hardcodear credenciales WiFi en el código `.c`.
```c
load_config_from_nvs(); // Lee SSID, Pass, MQTT User desde Flash
```
Esto permite usar un script de provisión (`provision_device.py`) para configurar 1000 dispositivos sin recompilar el firmware.

### D. Heartbeat (Latido)
¿Cómo sabemos si el gateway sigue vivo si no hay beacons cerca?
La tarea `heartbeat_counter_task` envía un mensaje especial cada 60s:
`{"status": "online", "uptime": 12345}`.
Si la nube deja de recibirlo, puede disparar una alarma de "Gateway Caído".

---

## 3. Flujo de Datos

1.  **Aire:** iBeacon emite UUID `AA...FF`.
2.  **Radio:** ESP32 detecta y filtra por Manufacturer ID (Apple).
3.  **Cola:** Copia datos a estructura `ble_msg_t` y envía a Queue.
4.  **Procesador:** Tarea MQTT despierta, toma el mensaje.
5.  **JSON:** Crea payload `{"mac": "...", "rssi": -60, "uuid": "..."}`.
6.  **TLS:** Encripta payload con AES/RSA.
7.  **TCP/IP:** Envía paquete al broker HiveMQ.

---

## 4. Estudio de Mercado y Viabilidad de Producto

Este código representa un **Producto Mínimo Viable (MVP)** completo para un Gateway IIoT. Es la pieza de hardware que habilita la nube.

### 🏭 Mercado Industrial (Cold Chain Logistics)
*   **Producto:** "Gateway de Cadena de Frío para Transporte".
*   **Problema:** Pérdida de cargas farmacéuticas o alimentarias por fallos en refrigeración.
*   **Solución:** Instalar este Gateway en la cabina del camión (alimentado por el vehículo). En la carga, sensores iBeacon baratos de temperatura.
*   **Flujo:** El Gateway recoge datos de 50 sensores en la caja, filtra por umbrales y sube las alertas por 4G (vía WiFi hotspot) a la nube de AWS/Azure/HiveMQ.
*   **Valor:** Trazabilidad en tiempo real, cumplimiento normativo y reducción de mermas.

### 🏥 Mercado Salud (Hospital 4.0)
*   **Producto:** "Hub de Monitorización de Pacientes en Planta".
*   **Escenario:** Habitaciones de hospital.
*   **Funcionamiento:** Un Gateway en el techo de cada habitación. Recibe datos de parches BLE de temperatura y ritmo cardíaco de los pacientes, así como la ubicación de sillas de ruedas y bombas de infusión (Asset Tracking).
*   **Valor:** Centralización de datos de enfermería ("Nursing Station Dashboard") sin cablear a los pacientes.

### 🏢 Mercado Facility Management (Edificios Inteligentes)
*   **Producto:** "Puente de Calidad de Aire".
*   **Concepto:** Gateways distribuidos en oficinas grandes.
*   **Funcionamiento:** Sensores BLE de CO2, humedad y ocupación (PIR) en las mesas. El Gateway sube todo a la nube para optimizar el HVAC (Aire Acondicionado) automáticamente.
*   **Ahorro:** Reducción del 20-30% en factura energética al no climatizar zonas vacías.


---
*Autor: Guía generada por Asistente Gemini bajo el rol de Arquitecto de Sistemas.*
