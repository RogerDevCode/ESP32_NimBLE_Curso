# 🛡️ Safety-Critical BLE Client (Central)

## 1. Propósito del Proyecto
Implementar un **Cliente Central Autónomo** capaz de operar sin intervención humana. Su objetivo es escanear el entorno RF, discriminar ruido, identificar un servidor específico mediante UUID (firma criptográfica débil), conectarse y mantener la suscripción a datos críticos, recuperándose automáticamente ante desconexiones.

## 2. Arquitectura y Funcionamiento
El cliente actúa como una **Máquina de Estados Finita (FSM)** implícita gestionada por eventos GAP:
1.  **Scan (Escaneo Pasivo):** Busca paquetes de anuncio.
2.  **Filter (Filtrado):** Analiza el payload en busca del UUID de servicio objetivo de 128 bits.
3.  **Connect & Subscribe:** Establece conexión y escribe en el descriptor CCCD (Client Characteristic Configuration Descriptor) para activar notificaciones.
4.  **Watchdog Monitor:** Una tarea paralela vigila que el flujo de eventos no se detenga.

## 3. Estructuras Críticas y Mejoras

### A. Filtrado por UUID (Targeted Scanning)
El enfoque ingenuo conecta al primer dispositivo que ve. Esto es peligroso en entornos ruidosos.
**Solución:** Parsing del campo `BLE_HS_ADV_TYPE_UUID128_CMP` o `INC` dentro del evento de descubrimiento.
```c
// Solo conectar si el UUID coincide byte a byte
if (ble_uuid_cmp(&fields.uuids128[i].u, &gatt_svr_svc_uuid.u) == 0) {
    ble_gap_connect(...);
}
```

### B. Auto-Recuperación (Infinite Loop Safe)
Si el servidor se apaga, el cliente no debe morir ni lanzar excepciones.
**Solución:** Manejo explícito del evento `BLE_GAP_EVENT_DISCONNECT`.
```c
case BLE_GAP_EVENT_DISCONNECT:
    ESP_LOGW(TAG, "Disconnected. Rescanning...");
    conn_handle = BLE_HS_CONN_HANDLE_NONE;
    ble_app_scan(); // Reinicia el ciclo inmediatamente
    return 0;
```

### C. Monitor de Salud (Watchdog Task)
A diferencia del servidor, el cliente es reactivo (callbacks). Si los callbacks dejan de llegar, el watchdog del IDLE task podría no saltar, pero la lógica de negocio estaría muerta.
**Solución:** Una tarea `monitor_task` dedicada que asegura que el RTOS sigue planificando tareas correctamente.

## 4. Guía para Próximos Proyectos
1.  **Coherencia de UUIDs:** Asegura que `gatt_svr_svc_uuid` en el cliente sea idéntico al del servidor (copia exacta de la estructura `ble_uuid128_t`).
2.  **Latencia vs. Consumo:** Ajusta `conn_params.itvl_min/max`.
    -   *Baja Latencia:* 6 (7.5ms) - 12 (15ms).
    -   *Bajo Consumo:* 80 (100ms) - 160 (200ms).
    -   En este proyecto usamos modo **Baja Latencia** (~20ms) para garantizar la entrega rápida de alertas.
3.  **Lectura vs. Notificación:** Prefiere siempre **Notificaciones**. El *Polling* (leer activamente) satura la radio y gasta batería; la notificación es "Push" y mucho más eficiente.
