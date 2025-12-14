#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "driver/rtc_io.h"
#include "soc/rtc.h"

static const char *TAG = "DEEP_SLEEP_APP";

// Variable en memoria RTC (Slow Memory).
// Esta memoria NO se borra durante el Deep Sleep, solo con un Reset físico o corte de energía.
RTC_DATA_ATTR static int boot_count = 0;
RTC_DATA_ATTR static struct timeval sleep_enter_time;

void app_main(void)
{
    // 1. Incrementar contador de arranques persistente
    boot_count++;
    ESP_LOGI(TAG, "Boot count: %d", boot_count);

    // 2. Analizar la causa del despertar
    struct timeval now;
    gettimeofday(&now, NULL);
    int sleep_time_ms = (now.tv_sec - sleep_enter_time.tv_sec) * 1000 + (now.tv_usec - sleep_enter_time.tv_usec) / 1000;

    switch (esp_sleep_get_wakeup_cause()) {
        case ESP_SLEEP_WAKEUP_TIMER:
            ESP_LOGI(TAG, "Despertado por TIMER. Dormí por %d ms", sleep_time_ms);
            break;
        case ESP_SLEEP_WAKEUP_UNDEFINED:
            ESP_LOGI(TAG, "Reset de sistema o primer arranque (Power On Reset)");
            break;
        default:
            ESP_LOGI(TAG, "Despertado por causa no manejada: %d", esp_sleep_get_wakeup_cause());
            break;
    }

    // 3. Simular Tarea Crítica (ej: Lectura de sensor + Envío BLE/MQTT)
    // En un escenario real, aquí iría la lógica de negocio.
    ESP_LOGI(TAG, "Ejecutando tarea crítica (Simulación 2s)...");
    vTaskDelay(pdMS_TO_TICKS(2000));

    // 4. Preparación para dormir (Optimización de Energía)
    // Es buena práctica aislar GPIOs para evitar fugas de corriente
    // rtc_gpio_isolate(GPIO_NUM_12); // Ejemplo

    // 5. Configurar fuente de despertar (Wakeup Source)
    const int wakeup_time_sec = 5;
    ESP_LOGI(TAG, "Configurando timer para despertar en %d segundos...", wakeup_time_sec);
    esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000);

    // Guardar tiempo antes de dormir para calcular duración al despertar
    gettimeofday(&sleep_enter_time, NULL);

    // 6. Entrar en Deep Sleep
    ESP_LOGI(TAG, "Entrando en Deep Sleep. Hasta luego.");
    
    // Importante: esp_deep_sleep_start() no retorna. El chip se reinicia al despertar.
    esp_deep_sleep_start();
}
