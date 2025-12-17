# GUÍA TÉCNICA: Servidor BLE Notificaciones (Telemetría)

## 1. Introducción: El Arte de no Preguntar
En los primeros ejemplos, para saber un valor, el cliente tenía que leer ("Polling").
*   Cliente: "¿Qué temperatura hace?" -> Servidor: "20°C"
*   Cliente: "¿Qué temperatura hace?" -> Servidor: "20°C"
*   Cliente: "¿Qué temperatura hace?" -> Servidor: "21°C"

Esto es ineficiente. Gasta batería y ocupa ancho de banda.
Las **Notificaciones (Notify)** invierten el control:
*   Cliente: "Avísame cuando cambie".
*   Servidor: (Silencio...)
*   Servidor: "¡21°C!"

Este proyecto implementa un servidor que genera datos periódicos y los empuja (push) al cliente.

---

## 2. Análisis del Código Crítico

### A. La Tarea de Telemetría
Necesitamos un generador de datos independiente.
```c
void notify_task(void *pvParameters) {
    while(1) {
        // Solo enviamos si hay alguien escuchando
        if (g_conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            counter++;
            // Empaquetamos el contador (4 bytes)
            struct os_mbuf *om = ble_hs_mbuf_from_flat(&counter, sizeof(counter));
            
            // Enviamos
            ble_gatts_notify_custom(g_conn_handle, val_handle, om);
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Cada 1 segundo
    }
}
```
**Optimización:** Observa que comprobamos `g_conn_handle`. Si no hay cliente, no gastamos ciclos de CPU ni memoria intentando enviar paquetes al vacío.

### B. Gestión de Conexiones
Para saber si "hay alguien escuchando", necesitamos rastrear las conexiones.
```c
case BLE_GAP_EVENT_CONNECT:
    g_conn_handle = event->connect.conn_handle; // ¡Alguien entró!
    break;

case BLE_GAP_EVENT_DISCONNECT:
    g_conn_handle = BLE_HS_CONN_HANDLE_NONE; // Se fue.
    start_advertising(); // Volver a anunciarse para el siguiente.
    break;
```

---

## 3. Flujo de Eventos

1.  **Suscripción:** El cliente se conecta y escribe en el CCCD para activar notificaciones. (NimBLE maneja esto internamente, pero es un requisito previo).
2.  **Generación:** `notify_task` despierta cada segundo.
3.  **Envío:** NimBLE toma los datos, los fragmenta si es necesario (aunque 4 bytes caben de sobra) y los pone en la cola de transmisión de la radio.
4.  **Recepción:** El cliente recibe el dato instantáneamente sin haberlo solicitado.

---

## 4. Estudio de Mercado y Viabilidad de Producto

El modelo de **Notificaciones (Push)** es el estándar de oro para dispositivos alimentados por batería que requieren transmisión de datos en tiempo casi real.

### 🩺 Mercado Salud (Wearables Clínicos)
*   **Producto:** "Holter Cardíaco Desechable".
*   **Concepto:** Un parche adhesivo que el paciente lleva por 7 días.
*   **Funcionamiento:** Detecta arritmias localmente. Cuando ocurre un evento, despierta la radio y envía una ráfaga de notificaciones con el ECG de los últimos 30 segundos al móvil del usuario.
*   **Viabilidad:** Alta demanda post-Covid. El uso de notificaciones en lugar de streaming continuo permite que una batería de botón dure toda la semana.

### 🎮 Mercado Entretenimiento (Gaming)
*   **Producto:** "Guante de Realidad Virtual (VR Glove)".
*   **Concepto:** Un guante con sensores de flexión en los dedos e IMU (acelerómetro).
*   **Tecnología:** Envía la posición de cada dedo como notificación a 60Hz.
*   **Por qué Notify:** La latencia es crítica. Un sistema de lectura (Polling) introduciría un retraso inaceptable ("lag") entre mover la mano y verla moverse en el juego. Las notificaciones BLE de baja latencia son la única opción viable estándar.

### 🏭 Mercado Industrial (Sensórica Crítica)
*   **Producto:** "Detector de Fugas de Gas".
*   **Concepto:** Sensor de Metano/Propano instalado en válvulas remotas.
*   **Funcionamiento:** Silencio absoluto (ahorro de energía) mientras todo está bien. Si detecta > 5% LEL (Límite Explosivo), envía notificación de ALARMA PRIORITARIA.
*   **Valor:** Elimina el coste de inspecciones manuales. La inmediatez de la notificación puede prevenir explosiones.


---
*Autor: Guía generada por Asistente Gemini bajo el rol de Arquitecto de Sistemas.*
