# Proyecto 11: Hibernación Profunda (Deep Sleep)

Este proyecto demuestra cómo implementar un ciclo de vida eficiente energéticamente utilizando el modo **Deep Sleep** del ESP32.

## Conceptos Clave para el Profesional

### 1. Ciclo de Vida de Ejecución
A diferencia de un bucle `while(1)` estándar, una aplicación con Deep Sleep funciona por "ciclos de arranque":
1.  **Boot:** El sistema arranca desde cero (CPU reset).
2.  **Active:** Ejecuta la tarea (leer sensor, enviar dato).
3.  **Sleep:** Apaga casi todo (CPU, RAM, WiFi/BT) excepto el controlador RTC.

### 2. Memoria RTC (Retention Memory)
La memoria RAM estándar (SRAM) pierde su contenido al entrar en Deep Sleep. Para mantener estado entre ciclos (ej: contadores, flags, colas de envío pendientes), usamos la memoria RTC.
*   **En C:** Usamos el atributo `RTC_DATA_ATTR` antes de la definición de la variable.
*   **Limitación:** Son solo 8KB en el ESP32 estándar. Úsala con sabiduría.

### 3. Pipeline de Datos y Tiempos
Para un sistema de tracking o sensado, el *timeline* típico es:

| Tiempo (ms) | Acción | Consumo Aprox. |
|:-----------:|:-------|:---------------|
| 0 - 100 | Bootloader + Inicialización HW | ~30-50 mA |
| 100 - 150 | Lectura de Sensores | ~10 mA |
| 150 - 2000 | Stack WiFi/BLE + Envío | ~100-240 mA |
| 2000+ | Deep Sleep | **~10 µA** |

**Objetivo:** Minimizar el tiempo en "Active" para maximizar la vida de la batería.

## Compilación y Ejecución

1.  Navega al directorio:
    ```bash
    cd 11_deep_sleep
    ```
2.  Compila y flashea:
    ```bash
    idf.py flash monitor
    ```

## Qué observar
*   El `Boot count` incrementará en cada ciclo, demostrando que la variable `boot_count` sobrevive al reinicio.
*   El mensaje "Despertado por TIMER" confirmará que el RTC Timer funcionó.
*   Observa cómo el programa "termina" y vuelve a empezar, en lugar de quedarse en un bucle.
