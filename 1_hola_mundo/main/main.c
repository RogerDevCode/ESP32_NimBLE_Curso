/*
 * -----------------------------------------------------------------------------
 * Proyecto: 1 - "Hola Mundo" con CMake y ESP-IDF
 * Autor: Roger Gallegos (adaptado por IA)
 * Fecha: 08-12-2025
 *
 * Objetivo:
 *    Asegurar que el entorno de desarrollo ESP-IDF esté correctamente configurado.
 *    Compilar, flashear y monitorizar un programa básico que hace parpadear un LED.
 *    Este es el punto de partida fundamental para cualquier proyecto con ESP-IDF.
 *
 * Componentes Clave de ESP-IDF/FreeRTOS Utilizados:
 *  - #include "freertos/FreeRTOS.h": Cabecera principal del sistema operativo en tiempo real.
 *  - #include "freertos/task.h": API para la gestión de tareas (crear, eliminar, retardar).
 *  - #include "driver/gpio.h": API para controlar los pines de Entrada/Salida de Propósito General.
 *  - xTaskCreate(): Función para crear una nueva tarea.
 *  - vTaskDelay(): Función para pausar la ejecución de una tarea por un número de "ticks" del sistema.
 *  - pdMS_TO_TICKS(): Macro para convertir milisegundos a ticks de FreeRTOS, asegurando portabilidad.
 *
 * Lógica Principal:
 *  1. La función `app_main` es el punto de entrada de la aplicación.
 *  2. Dentro de `app_main`, se configura el pin del LED como una salida digital.
 *  3. Se crea una tarea separada llamada `led_blink_task`. FreeRTOS se encargará de ejecutarla.
 *  4. La tarea `led_blink_task` entra en un bucle infinito donde:
 *     a. Enciende el LED.
 *     b. Imprime un mensaje en la consola.
 *     c. Espera 500 milisegundos.
 *     d. Apaga el LED.
 *     e. Imprime otro mensaje.
 *     f. Espera otros 500 milisegundos.
 *     g. Se repite.
 * -----------------------------------------------------------------------------
*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

// Define el pin GPIO donde está conectado el LED.
// En la mayoría de las placas ESP32 DevKit, es el GPIO 2.
#define BLINK_GPIO 2

// Tag para los mensajes de log, útil para depurar.
static const char *TAG = "HOLA_MUNDO";

// --- Declaración de Funciones de Usuario ---
void led_blink_task(void *pvParameter);

// --- Punto de Entrada Principal ---
void app_main(void)
{
    ESP_LOGI(TAG, "Configurando el hardware.");

    // Configura el pin del LED como una salida.
    // gpio_reset_pin resetea la configuración a un estado conocido.
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    ESP_LOGI(TAG, "Creando la tarea de parpadeo.");
    
    // Crea la tarea que se encargará del parpadeo del LED.
    // Parámetros:
    // 1. Puntero a la función de la tarea.
    // 2. Nombre de la tarea (para depuración).
    // 3. Tamaño de la pila (stack size) en palabras. 2048 es seguro para una tarea simple.
    // 4. Parámetros para la tarea (no usamos ninguno, por eso NULL).
    // 5. Prioridad de la tarea (1 es una prioridad baja).
    // 6. Handle de la tarea (no lo necesitamos, por eso NULL).
    xTaskCreate(led_blink_task, "led_blink_task", 2048, NULL, 1, NULL);
    
    ESP_LOGI(TAG, "Configuración completada. `app_main` puede terminar ahora.");
}

// --- Implementación de Funciones de Usuario ---

/**
 * @brief Tarea que hace parpadear un LED indefinidamente.
 * 
 * @param pvParameter Parámetros pasados durante la creación de la tarea (no se utiliza).
 */
void led_blink_task(void *pvParameter)
{
    ESP_LOGI(TAG, "Iniciando parpadeo del LED...");
    int level = 0;

    while (1) { // Bucle infinito, la tarea nunca debe terminar.
        
        // Cambia el nivel del LED (0 -> 1, 1 -> 0)
        level = !level;
        gpio_set_level(BLINK_GPIO, level);

        if(level) {
            ESP_LOGI(TAG, "Encendiendo LED");
        } else {
            ESP_LOGI(TAG, "Apagando LED");
        }

        // --- Código Crítico ---
        // Pausa la tarea actual por 500 milisegundos.
        // Es CRÍTICO usar vTaskDelay en lugar de otras funciones de "sleep"
        // porque vTaskDelay le permite al planificador de FreeRTOS ejecutar otras tareas
        // mientras esta está "dormida". Si no se usa, se puede bloquear el sistema.
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}
