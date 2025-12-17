# GUÍA TÉCNICA: Deep Sleep (Ultra Bajo Consumo)

## 1. Introducción: Durmiendo para Sobrevivir
Un ESP32 activo consume ~100-240mA. Una batería de 2000mAh duraría 10 horas. Inviable para un sensor remoto.
En **Deep Sleep**, el consumo baja a ~10µA (microamperios). La misma batería dura **años**.

Este proyecto simula el ciclo de vida de un sensor comercial:
1.  Despertar.
2.  Medir/Transmitir rápido.
3.  Dormir profundo.

---

## 2. Análisis del Código Crítico

### A. Memoria RTC (La que no olvida)
Al despertar de Deep Sleep, la RAM normal se pierde (se borra). La memoria RTC se mantiene alimentada.
```c
RTC_DATA_ATTR static int boot_count = 0;
```
Usamos `RTC_DATA_ATTR` para guardar variables que deben sobrevivir al sueño (ej. contador de paquetes, último estado).

### B. Causa del Despertar
Al iniciar `app_main`, debemos saber: ¿Es la primera vez (Power On) o me despertó el temporizador?
```c
switch (esp_sleep_get_wakeup_cause()) {
    case ESP_SLEEP_WAKEUP_TIMER: // Ya he funcionado antes
    case ESP_SLEEP_WAKEUP_UNDEFINED: // Acaban de poner la pila
}
```

### C. Configuración de Sueño
Configuramos el disparador (Trigger) y ejecutamos la sentencia de muerte.
```c
esp_sleep_enable_timer_wakeup(5 * 1000000); // 5 segundos en microsegundos
esp_deep_sleep_start(); // ¡ADIÓS! (El código se detiene aquí)
```
**Nota:** Después de esta línea, el procesador se apaga. Al despertar, **no** continúa en la línea siguiente. Empieza de cero desde `app_main`.

---

## 3. Flujo de Vida

1.  **Boot:** CPU arranca.
2.  **Check:** ¿Vengo de dormir?
    *   **Si:** Incremento `boot_count`.
    *   **No:** Inicializo variables a 0.
3.  **Trabajo:** Leo sensor (simulado 2s).
4.  **Shutdown:** Apago periféricos (WiFi/BT) para no gastar durante el apagado.
5.  **Sleep:** Apago CPU. Solo queda encendido el Timer de Ultra Bajo Consumo (ULP).
6.  **Wake:** Timer llega a 0 -> Genera Reset -> Vuelta al paso 1.

---

## 4. Estudio de Mercado y Viabilidad de Producto

El Deep Sleep no es un producto en sí, es la **tecnología habilitadora** para todo el mercado de "Deploy & Forget" (Desplegar y Olvidar). Sin esto, el IoT masivo es imposible por costes de mantenimiento (cambio de baterías).

### 🌳 Mercado Agro-Tech (Agricultura de Precisión)
*   **Producto:** "Estaca de Suelo Conectada".
*   **Concepto:** Un tubo PVC clavado en el campo. Mide NPK (Nutrientes), pH y Humedad.
*   **Ciclo de Energía:** Despierta 1 vez por hora → Lee sensores (100ms) → Transmite LoRaWAN/BLE (500ms) → Duerme 59 minutos.
*   **Viabilidad:** Autonomía de 2 a 5 años con baterías de litio primarias (Li-SOCl2). Permite monitorizar hectáreas sin tirar un solo cable.

### 🚗 Mercado Smart City (Parking)
*   **Producto:** "Sensor de Plaza de Aparcamiento".
*   **Concepto:** Una "tachuela" pegada al asfalto en cada plaza.
*   **Funcionamiento:** Sensor magnetómetro detecta si hay un coche encima (cambio en campo magnético). Al cambiar el estado (entra/sale coche), despierta y avisa. Heartbeat cada 24h.
*   **Valor:** Apps de "Encuentra tu parking" en tiempo real y fiscalización de zonas azules.

### 📦 Mercado Logística (Container Tracking)
*   **Producto:** "Caja Negra de Contenedor".
*   **Concepto:** Dispositivo soldado al contenedor marítimo.
*   **Funcionamiento:** Despierta cada 6 horas para intentar enviar posición y estado (golpes, apertura de puertas).
*   **Reto:** Debe sobrevivir meses en el mar sin recarga. La gestión de Deep Sleep y memoria RTC es crítica para no perder el log de eventos si no hay cobertura.


---
*Autor: Guía generada por Asistente Gemini bajo el rol de Arquitecto de Sistemas.*
