# 🛡️ Safety-Critical BLE Server (Notify Only)

## 1. Propósito
Ejemplo canónico y simplificado de un **Servidor de Telemetría**.
A diferencia de los proyectos complejos (Lazo Cerrado), este módulo se enfoca exclusivamente en la transmisión unidireccional eficiente de datos (Notificaciones) desde el periférico hacia el central.

## 2. Diferencias con Proyectos Anteriores
- **vs Proyecto 5:** Código más limpio, sin lógica de "Hola Mundo" mezclada.
- **vs Proyecto 6:** Sin lógica de escritura ni semáforos de sincronización. Solo "Fire and Forget" seguro.

## 3. Arquitectura
- **UUIDs:** Definidos en Stack para evitar *LoadProhibited*.
- **Watchdog:** Tarea `notify` protegida por TWDT (10s).
- **Transporte:** Notificaciones GATT (Push) vs Lectura (Pull).

## 4. Uso
Compilar y flashear en cualquier ESP32. Puede ser leído por el Cliente Genérico del Proyecto 5 o Apps como nRF Connect.
