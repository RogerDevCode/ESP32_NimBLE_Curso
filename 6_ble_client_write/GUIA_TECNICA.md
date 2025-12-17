# GUÍA TÉCNICA: Cliente BLE Escritura (Lazo Cerrado)

## 1. Introducción: Control Remoto Seguro
Leer es pasivo. Escribir es actuar.
Este proyecto implementa un **Controlador de Lazo Cerrado**. No solo enviamos una orden ("Enciende la luz"), sino que esperamos una confirmación del actuador ("Luz encendida").

**Filosofía de Seguridad:** Una orden no verificada es una orden fallida.

---

## 2. Análisis del Código Crítico

### A. La Máquina de Estados del Controlador
Usamos una tarea dedicada (`control_task`) separada de los callbacks de BLE. Esto evita bloquear el stack de comunicaciones.

```c
void control_task(void *param) {
    while (1) {
        // 1. Alternar estado deseado (0 -> 1 -> 0)
        target_state = !target_state;
        
        // 2. ENVIAR (Write)
        ble_gattc_write_flat(conn_handle, attr_handle, &target_state, ...);
        
        // 3. ESPERAR CONFIRMACIÓN (Wait)
        // Usamos un semáforo binario. Si el servidor no notifica en 2s, fallamos.
        if (xSemaphoreTake(feedback_sem, pdMS_TO_TICKS(2000)) == pdTRUE) {
            ESP_LOGI(TAG, "Confirmado.");
        } else {
            ESP_LOGE(TAG, "TIMEOUT: El servidor no respondió.");
            // Aquí iría lógica de reintento o emergencia
        }
        
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
```

### B. La Recepción de la Confirmación (Notificación)
El servidor (Proyecto 6 Servidor) debe estar programado para enviar una **Notificación** inmediatamente después de procesar la escritura.
```c
// En el callback gap_event:
case BLE_GAP_EVENT_NOTIFY_RX:
    uint8_t val = event->notify_rx.om->om_data[0];
    if (val == pending_val_to_verify) {
        xSemaphoreGive(feedback_sem); // ¡Despierta a la tarea de control!
    }
```

### C. Configuración de Suscripción (CCCD)
Antes de poder recibir notificaciones, el cliente **debe** escribir en el descriptor especial "Client Characteristic Configuration Descriptor" (CCCD).
```c
uint16_t cccd_val = BLE_GATT_CHR_PROP_NOTIFY; // 0x0001
ble_gattc_write_flat(..., cccd_handle, &cccd_val, ...);
```
Si olvidas este paso, el servidor nunca enviará nada, aunque te conectes.

---

## 3. Pruebas y Validación (Requiere 2 ESP32)
1.  **Dispositivo A (Servidor):** Debe ser capaz de recibir escrituras y notificar cambios (Ver Proyecto 6 Server).
2.  **Dispositivo B (Cliente):** Este proyecto.
3.  **Acción:**
    *   B se conecta a A.
    *   B escribe "1".
    *   A enciende LED y notifica "1".
    *   B recibe "1", desbloquea el semáforo y duerme 5s.
    *   B escribe "0"...

---
*Autor: Guía generada por Asistente Gemini bajo el rol de Arquitecto de Sistemas.*

## 4. Estudio de Mercado y Viabilidad de Producto

Este es el código de la **Acción Remota**. Aquí es donde el IoT deja de ser observador y pasa a controlar el mundo físico.

### 🏠 Mercado Doméstico (Acceso Seguro)
*   **Producto:** "Llave Digital para AirBnB".
*   **Concepto:** El inquilino descarga una App (o usa un llavero físico ESP32). El código implementa la escritura de un token criptográfico (como password) en la característica de la cerradura.
*   **Lazo Cerrado:** La cerradura intenta abrir el pestillo mecánico. Si lo logra (fin de carrera activado), envía la notificación "ABIERTO". La App muestra verde. Si el pestillo se traba, envía "ERROR_JAMMED".
*   **Valor:** Elimina la gestión física de llaves.

### 🚗 Mercado Automotriz (Aftermarket)
*   **Producto:** "Control de Suspensión Neumática".
*   **Concepto:** Controlar la altura de un vehículo 4x4 desde fuera.
*   **Funcionamiento:** El usuario pulsa "SUBIR" en el mando. El cliente escribe el comando. El controlador de las válvulas de aire ejecuta y monitorea los sensores de presión. Solo cuando la presión llega al objetivo, notifica "ALTURA_ALCANZADA".
*   **Seguridad:** El lazo cerrado es vital para no reventar las bolsas de aire por sobrepresión si la comunicación se corta.

### 🏭 Mercado Industrial (Robótica Colaborativa)
*   **Producto:** "Botonera de Parada Inalámbrica (Wireless E-Stop)".
*   **Concepto:** Un botón de paro de emergencia que lleva el supervisor colgado al cuello.
*   **Funcionamiento:** El dispositivo mantiene un "Heartbeat" de escritura constante con la máquina. Si escribe "PARO", la máquina se detiene. Pero más importante: si la máquina deja de recibir escrituras (pérdida de enlace) o el cliente deja de recibir confirmaciones (lazo roto), la máquina entra en modo seguro (Safe State) automáticamente.

