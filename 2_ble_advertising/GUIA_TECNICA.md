# GUÍA TÉCNICA: Publicidad BLE (Advertising) con NimBLE

## 1. Introducción: Gritando al Vacío
En el mundo de Bluetooth Low Energy (BLE), antes de que exista una conexión, existe el **Advertising** (Anuncio). Es el acto de un dispositivo gritando periódicamente: *"¡Estoy aquí! ¡Me llamo ESP32 y puedo hacer estas cosas!"*.

Este proyecto abandona el stack pesado original de Espressif (Bluedroid) en favor de **Apache NimBLE**, una pila BLE mucho más eficiente en uso de RAM y Flash, ideal para proyectos profesionales.

---

## 2. Análisis del Código Crítico

### A. La Estructura del Anuncio (Payload)
El "paquete" de anuncio es pequeño (máx 31 bytes). Debemos ser quirúrgicos con qué datos incluir.

```c
void start_advertising(void) {
    struct ble_hs_adv_fields fields = {0};
    
    // Configuración de Flags
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    
    // Configuración de iBeacon (Fabricante Apple)
    uint8_t mfg_data[25];
    // ... relleno de datos mágicos de iBeacon ...
    fields.mfg_data = mfg_data;
    fields.mfg_data_len = sizeof(mfg_data);

    ble_gap_adv_set_fields(&fields);
}
```
**Análisis:**
*   **Flags:** `DISC_GEN` significa "descubrible generalmente" (siempre visible). `BREDR_UNSUP` indica que NO soportamos Bluetooth Clásico (audio, etc.), solo BLE. Esto ahorra batería en el escáner del teléfono al no intentar negociar protocolos antiguos.
*   **Manufacturer Data:** Aquí "hackeamos" un poco insertando la estructura de iBeacon. Un iBeacon no es más que un patrón específico de bytes dentro del campo de "Datos de Fabricante" de un anuncio BLE estándar.

### B. El Motor de Sincronización
```c
void ble_app_on_sync(void)
{
    ble_hs_util_ensure_addr(0);
    start_advertising();
}
```
**Análisis:**
A diferencia de un programa secuencial, la pila BLE corre en paralelo. No podemos empezar a anunciar en `app_main`. Debemos esperar el evento de **Sincronización (`sync`)**.
1.  `ble_hs_util_ensure_addr(0)`: Si el dispositivo no tiene una dirección MAC pública estática, genera una aleatoria resoluble. Vital para la privacidad y funcionamiento.
2.  Solo cuando el hardware radio y el software stack están listos, llamamos a `start_advertising`.

### C. La Tarea Host (El Corazón)
```c
void ble_host_task(void *param)
{
    nimble_port_run(); // <--- Bucle infinito de eventos BLE
    nimble_port_freertos_deinit();
}
```
**Análisis:**
`nimble_port_run()` es una función bloqueante. Nunca retorna. Es el bucle que procesa todos los paquetes de radio entrantes y salientes. Por eso **debe** tener su propia tarea en FreeRTOS, separada de `app_main` y de cualquier lógica de sensores.

---

## 3. Flujo de Eventos

1.  **Inicio:** `app_main` inicializa la memoria NVS (necesaria para guardar claves de seguridad BLE) y el puerto NimBLE.
2.  **Lanzamiento:** Se crea la tarea `ble_host_task`.
3.  **Sincronización:** NimBLE termina de arrancar y llama al callback `ble_app_on_sync`.
4.  **Configuración:** Se construye el paquete de datos (iBeacon + Flags).
5.  **Emisión:** `ble_gap_adv_start` ordena al controlador de radio emitir ráfagas de RF en los canales 37, 38 y 39.
6.  **Repetición:** El hardware retransmite el paquete automáticamente según el intervalo configurado (160 * 0.625ms = 100ms), liberando a la CPU para otras cosas.

## 4. Pruebas y Validación
1.  Descarga la app **nRF Connect for Mobile** (Android/iOS).
2.  Compila y flashea.
3.  Abre nRF Connect y escanea.
4.  Busca un dispositivo llamado "ESP32-NimBLE" o que se identifique como un iBeacon con UUID `AA:BB:CC...`.
5.  Observa el RSSI (intensidad de señal). Al alejarte, debe bajar (ej. -80dBm).

---
*Autor: Guía generada por Asistente Gemini bajo el rol de Arquitecto de Sistemas.*

## 5. Estudio de Mercado y Viabilidad de Producto

El código de **Advertising** es la tecnología base de una industria de miles de millones de dólares: la localización en interiores y el marketing de proximidad.

### 🏢 Mercado Comercial (Retail & Marketing)
*   **Producto:** "Baliza de Ofertas (Virtual Coupon Beacon)".
*   **Concepto:** Pequeñas cajas pegadas en estanterías de supermercados. Transmiten un UID único. La App del supermercado en el teléfono del cliente detecta el ID y lanza una notificación: *"¡Estás frente a los vinos! 20% de descuento en Malbec hoy"*.
*   **Modelo de Negocio:** Venta del hardware (Beacons) + Suscripción mensual por la plataforma de gestión de campañas.

### 🏭 Mercado Industrial (Logística)
*   **Producto:** "Etiqueta de Activo Desechable (Asset Tag)".
*   **Concepto:** Modificar el código para dormir el 99% del tiempo y anunciar solo cada 5 segundos. Alimentado por una batería de botón (CR2032). Se adhiere a palets, contenedores o maquinaria costosa.
*   **Valor:** Permite realizar inventarios automatizados en segundos simplemente caminando por el almacén con un escáner, sin línea de vista (a diferencia del código de barras).

### 🏠 Mercado Doméstico (Seguridad)
*   **Producto:** "Llavero Anti-Pérdida".
*   **Concepto:** Un llavero que emite este advertising. Si tu teléfono deja de recibir la señal (RSSI baja o desconexión), la App suena una alarma. "Te estás dejando las llaves".
*   **Diferenciador:** Código abierto, compatible con Home Assistant para detectar presencia (ej. encender luces cuando el llavero llega a casa).

