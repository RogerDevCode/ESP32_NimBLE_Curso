# 🛡️ Safety-Critical BLE Write (Closed-Loop) - Client

Este documento complementa el README principal ubicado en el servidor.

## Lógica del Cliente (Controlador)

El cliente no es un simple disparador de comandos. Es un agente autónomo diseñado para asegurar que el estado del sistema remoto coincida con el objetivo deseado.

### Flujo de Control (Task: `control_task`)

1.  **Conexión:** Escanea y filtra por UUID específico.
2.  **Suscripción:** Se suscribe a notificaciones (CCCD) antes de intentar cualquier escritura.
3.  **Ciclo de Mando:**
    -   Define `target_state` (Toggle 0/1).
    -   Envía Escritura (`ble_gattc_write_flat`).
    -   **Bloquea** esperando semáforo `feedback_sem` (Timeout 2s).
4.  **Manejo de Errores:**
    -   Si el timeout expira: Reintenta la escritura.
    -   Si el valor notificado es incorrecto: Reintenta la escritura.
    -   Si se desconecta: Pausa y espera reconexión automática.

### Archivos Clave
-   `main/main.c`: Lógica completa de la máquina de estados.
-   `sdkconfig.defaults`: Configuración endurecida (Hardened) para Watchdogs y Stack Check.