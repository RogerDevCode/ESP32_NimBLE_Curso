# 📘 Curso Avanzado de ESP32: NimBLE, FreeRTOS y IoT Industrial

Este repositorio contiene la colección completa de proyectos prácticos y código fuente del curso de desarrollo profesional para **ESP32** utilizando el stack **NimBLE** (Bluetooth Low Energy ligero y eficiente) sobre **ESP-IDF v5.x**.

## 🎯 Objetivo Final del Curso

El propósito de esta ruta de aprendizaje es transformar al desarrollador en un **Arquitecto de Sistemas Embebidos**.

Al finalizar el recorrido, no solo sabrás "cómo encender un LED por Bluetooth", sino que habrás construido un **Gateway IoT de Grado Industrial** (Proyecto 10) capaz de:
1.  Escanear y filtrar cientos de sensores BLE (iBeacons) en tiempo real.
2.  Gestionar conexiones WiFi resilientes con reconexión automática.
3.  Transmitir datos de forma segura (TLS/SSL) a la nube mediante MQTT (HiveMQ Cloud).
4.  Operar 24/7 sin bloqueos gracias a la implementación de **Watchdogs, Manejo de Errores y Arquitectura Multitarea (FreeRTOS)**.

---

## 🚀 Índice Maestro de Proyectos

A continuación, se detalla la evolución paso a paso. Cada proyecto construye sobre el anterior.

| Proyecto | Descripción Técnica | Ejemplo en la Vida Real |
| :--- | :--- | :--- |
| **1_hola_mundo** | **Configuración del Entorno.** Verificación del toolchain, CMake y el sistema de logs del ESP32. | El "Check-list" de despegue antes de iniciar cualquier desarrollo de firmware. |
| **2_ble_advertising** | **El "Faro" Bluetooth.** Configuración del ESP32 como *Broadcaster*. Envía paquetes de publicidad sin conexión. | Un sensor de temperatura en un supermercado que emite su valor a cualquiera que pase cerca. |
| **3_ble_services_chars** | **Servidor GATT.** Definición de Servicios y Características. Estructura de datos interna del dispositivo. | El perfil de un Reloj Inteligente que expone batería, hora y pasos como datos legibles. |
| **4_ble_client_scan** | **Escáner (Observer).** El ESP32 busca dispositivos cercanos y filtra por nombre o RSSI (potencia de señal). | El sistema de tu teléfono buscando auriculares Bluetooth disponibles para emparejar. |
| **5_ble_client_connect** | **Cliente Central (Lectura).** Conexión activa a un servidor y lectura de características bajo demanda. | Tu teléfono conectándose al Reloj Inteligente para descargar el historial de sueño. |
| **6_ble_write** | **Control Remoto (Escritura).** Cliente y Servidor configurados para recibir comandos y actuar sobre el hardware. | Una App móvil que enciende/apaga una bombilla inteligente o abre una cerradura electrónica. |
| **7_ble_server_notify** | **Notificaciones en Tiempo Real.** El servidor "empuja" datos al cliente sin que este pregunte (ahorro de energía). | Un monitor cardíaco enviando cada latido al teléfono instantáneamente. |
| **8_ble_beacon_scanner** | **Rastreo de Activos.** Escáner especializado en detectar y decodificar el estándar *iBeacon* (Apple) y *Eddystone*. | Localización en interiores (aeropuertos/museos) o control de inventario en almacenes. |
| **9_ble_multi_scanner** | **Multitarea Avanzada.** Gestión de escaneos complejos y listas blancas en entornos saturados. | Un concentrador en una oficina que gestiona la presencia de docenas de empleados simultáneamente. |
| **10_ble_mqtt_gateway** | **👑 EL PROYECTO CAPSTONE.** Fusión de BLE + WiFi + MQTT + TLS. Arquitectura de seguridad y recuperación ante fallos. | Un Hub IoT en un hospital que recolecta datos de pacientes (BLE) y los sube a la nube (MQTT) de forma segura. |
| **11_deep_sleep** | **Eficiencia Energética.** Uso de modos de bajo consumo (Deep Sleep) y despertar por temporizador o GPIO. | Un sensor agrícola en el campo que debe funcionar 5 años con una sola batería. |

---

## 🛠️ Cómo Utilizar este Repositorio

Todos los proyectos están configurados para funcionar con **ESP-IDF v5.x** y han sido "endurecidos" (*hardened*) para producción.

### Requisitos Previos
*   ESP-IDF v5.0 o superior instalado.
*   Python 3.x.
*   Hardware: ESP32 o ESP32-S3.

### Compilación y Flash
Para probar cualquier proyecto (ejemplo: Proyecto 10), abre una terminal en la carpeta correspondiente y ejecuta:

1.  **Configurar el Target (Solo la primera vez):**
    ```bash
    idf.py set-target esp32s3  # o esp32, esp32c3
    ```

2.  **Configurar Credenciales (Si aplica):**
    ```bash
    idf.py menuconfig
    # Ir a "Project Configuration" para poner tus claves WiFi/MQTT
    ```

3.  **Compilar, Flashear y Monitorizar:**
    ```bash
    idf.py build flash monitor
    ```

---

## 🛡️ Arquitectura "High Reliability"

A diferencia de ejemplos básicos, los proyectos avanzados de este repositorio (especialmente el #10) implementan reglas estrictas de ingeniería de software:

*   **Watchdogs (WDT):** El sistema se reinicia automáticamente si una tarea se congela.
*   **Gestión de Errores:** No hay fallos silenciosos. Todo error crítico se reporta o se gestiona.
*   **Memoria Estática:** Uso preferente de memoria estática sobre dinámica para evitar fragmentación del Heap.
*   **Seguridad:** Uso de TLS/SSL para comunicaciones MQTT y sanitización de credenciales en los archivos de configuración.

---

> **Nota:** Este código es material educativo profesional. Úsalo como base para tus propios productos, pero recuerda siempre realizar auditorías de seguridad antes de desplegar en entornos comerciales.