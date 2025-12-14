# 🛡️ Safety-Critical BLE Server (GATT)

## 1. Propósito del Proyecto
Implementar un **Servidor GATT de Alta Disponibilidad** que actúa como nodo de telemetría industrial. Su función es emitir un "latido" (heartbeat) constante y seguro hacia un cliente autenticado, garantizando que el enlace de datos no se degrade con el tiempo ni colapse por errores de memoria.

## 2. Arquitectura y Funcionamiento
El sistema opera bajo un modelo **Event-Driven + RTOS Task**:
1.  **NimBLE Host Task:** Gestiona la pila de protocolos BLE (prioridad alta). Maneja conexiones y eventos GAP.
2.  **Notify Task (User Space):** Tarea independiente que genera datos.
    -   *Aislamiento:* No bloquea el stack BLE.
    -   *Seguridad:* Protegida por **Task Watchdog Timer (TWDT)**. Si esta tarea se cuelga por >10s, el sistema se reinicia.
3.  **Split Advertising:** Estrategia de anuncio dividido para cumplir estrictamente con el MTU de 31 bytes.

## 3. Estructuras Críticas y Mejoras (vs. Enfoque Ingenuo)

### A. Estrategia de Anuncio Dividido (Fix `BLE_HS_EMSGSIZE`)
En versiones anteriores, intentar enviar todo en un paquete causaba error.
**Solución:**
```c
// Paquete 1: Crítico para conexión (Flags + UUID)
fields.uuids128 = (ble_uuid128_t *)&gatt_svr_svc_uuid;
ble_gap_adv_set_fields(&fields);

// Paquete 2: Informativo (Nombre del Dispositivo)
rsp_fields.name = (uint8_t *)DEVICE_NAME;
ble_gap_adv_rsp_set_fields(&rsp_fields);
```

### B. Gestión de Memoria Defensiva (`os_mbuf`)
El stack NimBLE usa buffers encadenados (`mbufs`). Fallar al asignarlos es común bajo carga.
**Solución:**
```c
struct os_mbuf *om = ble_hs_mbuf_from_flat(payload, strlen(payload));
if (om == NULL) {
    // Manejo de error: No intentar enviar, liberar recursos si existen
    ESP_LOGE(TAG, "No memory for mbuf!");
    return; 
}
// Solo proceder si 'om' es válido
ble_gatts_notify_custom(..., om);
```

### C. Watchdog Integration
Evita "zombies" (tareas muertas pero sistema encendido).
**Implementación:**
```c
esp_task_wdt_add(NULL); // Registrar tarea actual
while(1) {
    esp_task_wdt_reset(); // "Patear" al perro
    // ... lógica ...
}
```

## 4. Guía para Próximos Proyectos
1.  **Nunca asumas que hay memoria:** Verifica siempre los punteros retornados por `ble_hs_mbuf_...`.
2.  **UUIDs Little Endian:** NimBLE invierte los bytes. Define tus UUIDs byte a byte o usa macros que lo inviertan, pero sé consistente entre cliente y servidor.
3.  **No bloquees Callbacks:** Los callbacks de `ble_gap_event` corren en la tarea del stack. Hazlos rápidos o delega a una cola (Queue).