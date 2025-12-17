# GUÍA TÉCNICA: Servidor BLE Escritura y Control

## 1. Introducción: Obedeciendo Órdenes
Hasta ahora, nuestros servidores solo mostraban datos ("Temperatura: 25°C"). Pero un sistema IoT real necesita **actuar**: encender una válvula, abrir una puerta, resetear un sistema.

Este proyecto implementa la capacidad de recibir comandos desde un cliente, validarlos y confirmar su ejecución. Es la contraparte necesaria del Cliente de Escritura (Proyecto 6 Cliente).

---

## 2. Análisis del Código Crítico

### A. Permisos de la Característica
Para permitir que alguien escriba, debemos declararlo explícitamente en la tabla GATT.
```c
.flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_NOTIFY,
```
**Análisis:**
*   **WRITE:** Permite al cliente enviar datos ("Write Request").
*   **NOTIFY:** Permite al servidor enviar actualizaciones sin preguntar.
*   **Combinación:** Juntos, permiten el control de "Lazo Cerrado".

### B. El Callback de Escritura (`gatt_svr_access`)
Aquí reside la seguridad del sistema.
```c
if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
    // 1. VALIDACIÓN DE LONGITUD
    // ¿Me envían 1 byte? Si me envían 0 o 20, es un ataque o error.
    if (OS_MBUF_PKTLEN(ctxt->om) != 1) return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    
    // 2. EXTRACCIÓN
    uint8_t val;
    ble_hs_mbuf_to_flat(ctxt->om, &val, 1, NULL);

    // 3. VALIDACIÓN DE RANGO (Opcional pero recomendada)
    if (val > 1) return BLE_ATT_ERR_VALUE_NOT_ALLOWED;

    // 4. ACCIÓN
    actuator_state = val;
    // (Aquí activaríamos un GPIO real: gpio_set_level(PIN, val))
    
    // 5. CONFIRMACIÓN (Notificación inmediata)
    notify_state();
    
    return 0; // Éxito (El stack envía "Write Response")
}
```

### C. Notificación de Estado (`notify_state`)
Es crucial informar a **todos** los clientes conectados (puede haber varios escuchando) que el estado ha cambiado.
```c
struct os_mbuf *om = ble_hs_mbuf_from_flat(&actuator_state, 1);
ble_gatts_notify_custom(g_conn_handle, val_handle, om);
```

---

## 3. Flujo de Eventos (Ciclo de Escritura)

1.  **Recepción:** La radio recibe un paquete `WRITE_REQ` con el valor `0x01`.
2.  **Callback:** NimBLE pausa el hilo principal y llama a `gatt_svr_access`.
3.  **Procesamiento:** Validamos y actualizamos la variable `actuator_state`.
4.  **Respuesta L2:** Al retornar `0`, NimBLE envía automáticamente un paquete `WRITE_RSP` (Ack).
5.  **Respuesta L7:** Llamamos a `notify_state` para enviar un paquete `HANDLE_VALUE_NTF` con el nuevo valor.

---

## 4. Casos de Uso Reales

### A. Domótica Segura (Smart Lock)
*   **Escenario:** Una cerradura inteligente.
*   **Flujo:** El usuario envía "ABRIR" (Write). La cerradura no solo abre, sino que notifica "ABIERTO" (Notify). La App del usuario muestra el candado verde solo cuando llega la notificación, no cuando se pulsó el botón.

### B. Control Industrial (PLC)
*   **Escenario:** Arrancar un motor trifásico.
*   **Flujo:** El operario envía "START". El ESP32 activa el contactor. El ESP32 lee el sensor de corriente para verificar que *realmente* arrancó. Si hay corriente, envía notificación "RUNNING". Si el contactor falló, envía "ERROR_STUCK".

---
*Autor: Guía generada por Asistente Gemini bajo el rol de Arquitecto de Sistemas.*
