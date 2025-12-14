# 🛡️ Safety-Critical BLE Write (Closed-Loop)

## 1. Propósito del Proyecto
Implementar un sistema de **Mando y Control (C2)** robusto sobre BLE. A diferencia de la telemetría simple (leer datos), este proyecto permite a un cliente (controlador) modificar el estado de un servidor (actuador) de manera segura.

**Filosofía:** "Trust but Verify".
En sistemas críticos, enviar un comando de escritura no es suficiente. Se requiere una confirmación positiva de que el actuador ha cambiado su estado físico.

## 2. Arquitectura de Lazo Cerrado
El sistema implementa un ciclo de verificación en 4 pasos:

1.  **Command (Cliente):** Envía `WRITE REQUEST` con el valor deseado (ej. `1` para ON).
2.  **Ack Nivel 2 (Stack BLE):** El stack confirma la recepción del paquete (Write Response).
3.  **Action (Servidor):** El servidor valida el dato, actualiza su variable de estado y (en un caso real) activaría un relé.
4.  **Verification (Servidor -> Cliente):** El servidor envía una **Notificación** inmediata con el nuevo estado. El cliente solo considera la operación "Exitosa" si el valor notificado coincide con el comando enviado.

## 3. Estructura de Proyectos

### 📂 `6_ble_server_write` (El Actuador)
-   **Rol:** Periférico GATT.
-   **Características:**
    -   Servicio UUID: `...0028`
    -   Característica UUID: `...0128` (Permisos: READ | WRITE | NOTIFY)
-   **Seguridad:** Validación estricta de payload (solo acepta 1 byte, valores 0 o 1).
-   **Estabilidad:** Uso de literales compuestos para Advertising (evita crashes de `LoadProhibited`).

### 📂 `6_ble_client_write` (El Controlador)
-   **Rol:** Central GATT.
-   **Lógica:** Máquina de estados con reintento persistente.
-   **Manejo de Eventos:**
    -   `BLE_GAP_EVENT_NOTIFY_RX`: Procesa la verificación del servidor.
    -   Uso de Semáforos (`feedback_sem`) para sincronizar la tarea de control con el evento de red.

## 4. Mejoras Críticas Implementadas

| Problema Previo | Solución Aplicada |
|-----------------|-------------------|
| **Crash al Iniciar:** `LoadProhibited` en `ble_gap_adv_set_fields`. | Uso de memoria local (Stack) para estructuras de UUID en lugar de punteros estáticos a Flash/DROM. |
| **Incertidumbre:** El cliente no sabía si el servidor ejecutó la orden. | Implementación de Lazo Cerrado (Write -> Wait Notify). |
| **Bloqueo:** Si el servidor no respondía, el cliente se colgaba. | Timeouts en semáforos y lógica de reintento infinita (Watchdog safe). |
| **Corrupción NVS:** Bloqueo al reiniciar por claves antiguas. | `CONFIG_BT_NIMBLE_NVS_PERSIST=n` en sdkconfig. |

## 5. Cómo Ejecutar

### Hardware Requerido
-   2x ESP32
-   Puerto USB0: Servidor
-   Puerto USB1: Cliente

### Comandos
```bash
# 1. Compilar y Flashear Servidor
cd 6_ble_server_write
idf.py build flash -p /dev/ttyUSB0

# 2. Compilar y Flashear Cliente
cd ../6_ble_client_write
idf.py build flash -p /dev/ttyUSB1

# 3. Monitorear (Script Python incluido)
python3 ../monitor_dual.py
```

### Salida Esperada
```text
[CLIENT] >>> COMMAND: Writing 1...
[SERVER] Write Received: 1
[SERVER] Notified State: 1
[CLIENT] RX NOTIFICATION (Handle 16)
[CLIENT] VERIFICATION RECEIVED: Device is now ON
[CLIENT] <<< SUCCESS: State 1 Confirmed.
```
