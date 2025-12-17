# GUÍA TÉCNICA: Hola Mundo en Sistemas Embebidos (ESP-IDF)

## 0. Introducción al Laboratorio y Entorno

Esta guía es el punto de entrada para el desarrollo profesional con ESP32. Antes de escribir código, establecemos los cimientos de nuestro entorno de trabajo.

### Lista de Materiales (BOM) - El Taller del Ingeniero
Para seguir este curso con rigor profesional, se requiere el siguiente equipamiento:

*   **Núcleo:** 2x ESP32-S3 DevKitC-1 (o ESP32 WROOM-32).
*   **Conectividad:** 1x Hub USB 3.0 con alimentación externa (Crucial para evitar *brownouts*).
*   **Cables:** 2x USB-A a USB-C de alta calidad y certificados para datos.
*   **Prototipado:**
    *   2x Protoboards MB-102.
    *   Juego de cables Jumper (M-M, M-H, H-H).
    *   Resistencias: 330Ω, 1kΩ, 4.7kΩ, 10kΩ.
    *   LEDs variados y Capacitores (100nF cerámico, 100uF electrolítico).
*   **Instrumentación:** Multímetro auto-rango y (opcional) Analizador Lógico 8ch.

---

## 1. Instalación del Entorno (Toolchain)

### Opción A: Linux / macOS (Recomendado)
El método para quienes buscan control total.

1.  **Prerrequisitos:** Instalar `git`, `wget`, `python3`, `cmake`, `ninja-build`.
    *   *Ubuntu:* `sudo apt-get install git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0`
2.  **Obtener ESP-IDF:**
    ```bash
    mkdir -p ~/esp && cd ~/esp
    git clone -b v5.3.1 --recursive https://github.com/espressif/esp-idf.git
    ```
3.  **Instalar Herramientas:**
    ```bash
    cd ~/esp/esp-idf
    ./install.sh esp32,esp32s3
    ```
4.  **Configurar Variables:**
    ```bash
    . $HOME/esp/esp-idf/export.sh
    ```

### Opción B: Windows
1.  Descargar el **ESP-IDF Online Installer** desde el sitio oficial de Espressif.
2.  Ejecutar e instalar versión `v5.3.1` (o superior) en `C:\Espressif`.
3.  Utilizar el acceso directo **"ESP-IDF 5.x CMD"** creado en el escritorio.

---

## 2. Análisis del Proyecto: "Hola Mundo" (Multitasking)

Este no es un simple script secuencial. Es la demostración de un sistema operativo en tiempo real (RTOS) gestionando recursos.

### Código Crítico y Explicación

#### A. El Punto de Entrada no es `main()`
```c
void app_main(void)
{
    // ... configuración ...
    xTaskCreate(led_blink_task, "led_blink_task", 2048, NULL, 1, NULL);
}
```
**Análisis:** En ESP-IDF, `app_main` es invocada por el sistema una vez que el hardware y el kernel de FreeRTOS están inicializados. Su propósito principal es **lanzar otras tareas**. Aquí, no nos quedamos en un bucle dentro de `app_main`; delegamos el trabajo a `led_blink_task`.

#### B. La Tarea Infinita
```c
void led_blink_task(void *pvParameter)
{
    while (1) {
        // ... lógica ...
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
```
**Análisis:**
1.  **`while(1)`:** Las tareas en sistemas embebidos deben ser bucles infinitos. Si una tarea retorna (llega al final de su función), el sistema colapsa a menos que se elimine explícitamente con `vTaskDelete(NULL)`.
2.  **`vTaskDelay` vs `sleep`:** Esta es la joya de los RTOS. Al llamar a `vTaskDelay`, la tarea le dice al planificador: *"Libero la CPU por 500ms"*. Durante este tiempo, el procesador puede entrar en bajo consumo, gestionar Wi-Fi o atender otras tareas. Un bucle `for` de retardo (busy-wait) bloquearía el sistema y desperdiciaría energía.

### Flujo de Eventos
1.  **Arranque:** El Bootloader carga la aplicación y salta a `app_main`.
2.  **Creación:** `app_main` solicita al kernel 2048 bytes de RAM para la pila (stack) de la nueva tarea.
3.  **Planificación:** El Scheduler de FreeRTOS toma el control. Alterna la ejecución.
4.  **Ejecución:**
    *   La tarea enciende el LED.
    *   Llama a `vTaskDelay` y entra en estado **Blocked**.
    *   (El procesador hace otras cosas o duerme).
    *   Pasados 500ms, el Tick Timer despierta la tarea (estado **Ready** -> **Running**).
    *   Apaga el LED y repite.

### Compilación y Ejecución
Desde la terminal configurada:
1.  `idf.py set-target esp32` (Configura el chip).
2.  `idf.py build` (Compila todo el sistema).
3.  `idf.py -p /dev/ttyUSB0 flash monitor` (Graba y abre la consola).

---
*Autor: Guía generada por Asistente Gemini bajo el rol de Arquitecto de Sistemas.*

## 3. Estudio de Mercado y Viabilidad de Producto

Aunque este código parece elemental, el concepto de **Task Scheduling** y **Watchdogs** es la base de productos de alta confiabilidad.

### 🏠 Mercado Doméstico (Smart Home)
*   **Producto:** "Rebooter Inteligente para Routers".
*   **Concepto:** Un dispositivo pequeño que se alimenta del USB del router. Ejecuta una tarea que hace ping a Google (vía WiFi) o verifica actividad. Si detecta congelamiento (el bucle de tarea falla o no hay red), corta la energía del router y la vuelve a activar (usando un relé o transistor).
*   **Valor:** Soluciona el problema de "Reinicia el router" automáticamente.

### 🏭 Mercado Industrial (IIoT 4.0)
*   **Producto:** "Hardware Watchdog Externo".
*   **Concepto:** En PLCs críticos o PCs industriales, a veces el software se cuelga pero la energía sigue. Este ESP32 actuaría como un supervisor externo. El PC debe enviarle un pulso cada segundo. Si el ESP32 deja de recibirlo (Timeout en `vTaskDelay`), activa una línea de Reset físico en el PC.
*   **Diferenciador:** Resiliencia extrema por hardware dedicado y barato (<$5 USD BOM).

