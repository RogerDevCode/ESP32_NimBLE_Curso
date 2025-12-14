# Notas de Sesión - Estado del Proyecto
*Fecha: 09-12-2025*

## Progreso Realizado
1.  **Tutorial Maestro (`tutorial.md`):**
    *   Guía completa de 11 proyectos (desde Hola Mundo hasta Deep Sleep).
    *   Enfoque en ingeniería robusta y "precisión quirúrgica".
2.  **Infraestructura Backend (`backend_setup.md`):**
    *   Guía para desplegar Mosquitto (Docker) y n8n.
    *   Configuración de seguridad y túnel Cloudflare.
3.  **Proyecto 11: Deep Sleep (`11_deep_sleep/`):**
    *   Código implementado y validado.
    *   Ciclo de vida eficiente con memoria RTC.
4.  **Proyecto 10: Gateway MQTT (`10_ble_mqtt_gateway/`):**
    *   **Refactorización Completa:** Implementación de colas FreeRTOS para desacoplar BLE/MQTT.
    *   **Robustez:** Exponential Backoff para reconexión WiFi y manejo de errores.
    *   **Configuración:** Migrado a **Kconfig**. Las credenciales ya no están en el código (`main.c`), sino que se gestionan vía `idf.py menuconfig`.

## Estado Actual
*   El **Proyecto 10** compila correctamente.
*   Está configurado para usar Kconfig.
*   **Pendiente:** Configurar las credenciales WiFi y MQTT reales antes de la prueba final de integración.

## Pasos para la Próxima Sesión
1.  Navegar al directorio del gateway:
    ```bash
    cd 10_ble_mqtt_gateway
    ```
2.  Configurar credenciales (WiFi SSID, Pass, Broker URL):
    ```bash
    idf.py menuconfig
    ```
    *Ir a "Gateway Configuration"*
3.  Flashear y monitorear:
    ```bash
    idf.py flash monitor
    ```
4.  Verificar recepción de datos en n8n.
