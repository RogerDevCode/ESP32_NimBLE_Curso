# GUÍA TÉCNICA: Multi-Scanner BLE Avanzado

## 1. Introducción: Escaneando Multitudes
Hasta ahora, hemos rastreado un solo dispositivo. Pero el mundo real está lleno de señales. Un gateway industrial debe rastrear docenas de sensores a la vez.

Este proyecto implementa un escáner con gestión de memoria dinámica (pero segura) para mantener una lista en vivo de múltiples dispositivos, registrando sus nombres y RSSI.

---

## 2. Análisis del Código Crítico

### A. Gestión de Memoria (Device Pool)
En sistemas embebidos, `malloc` es peligroso (fragmentación). Usamos un **Pool Estático**.
```c
#define MAX_DEVICES 20 
static scanned_device_t device_pool[MAX_DEVICES];
static bool pool_used[MAX_DEVICES];
```
Esto reserva memoria fija al compilar. Si necesitamos un nuevo dispositivo, buscamos un hueco libre en el array `pool_used`.

### B. Lista Enlazada (Linked List)
Aunque el almacenamiento es un array, la lógica usa una lista enlazada para recorrer los dispositivos activos eficientemente.
```c
typedef struct scanned_device {
    ble_addr_t addr;
    // ...
    struct scanned_device *next;
} scanned_device_t;
```

### C. Concurrencia (Mutex)
Tenemos dos tareas peleando por la misma memoria:
1.  **Callback BLE:** Escribe datos (Alta prioridad).
2.  **Report Task:** Lee datos para imprimir (Baja prioridad).

Sin protección, la tarea de reporte podría leer un dispositivo mientras el callback BLE lo está modificando, resultando en basura.
```c
if (xSemaphoreTake(list_mutex, portMAX_DELAY) == pdTRUE) {
    // Acceso seguro a la lista
    xSemaphoreGive(list_mutex);
}
```

---

## 3. Flujo de Eventos

1.  **Descubrimiento:** NimBLE detecta un paquete.
2.  **Bloqueo:** Callback intenta tomar el Mutex.
3.  **Búsqueda:** ¿Ya conocemos esta MAC?
    *   **Si:** Actualizamos RSSI y Nombre.
    *   **No:** Pedimos hueco al Pool. Si hay espacio, creamos nodo.
4.  **Desbloqueo:** Liberamos Mutex.
5.  **Reporte:** Cada 5 segundos, la tarea de reporte toma el Mutex, recorre la lista e imprime la tabla.

---

## 4. Estudio de Mercado y Viabilidad de Producto

La capacidad de rastrear múltiples objetivos simultáneamente es lo que diferencia un juguete de un **Sistema de Grado Enterprise**.

### 🐄 Mercado Agrícola (Smart Farming)
*   **Producto:** "Monitor de Rebaño (Livestock Tracker)".
*   **Problema:** Ganado disperso en grandes extensiones.
*   **Solución:** Collares BLE baratos en las vacas. Un escáner multi-dispositivo en los abrevaderos o zonas de paso.
*   **Funcionamiento:** Identifica qué animales han venido a beber hoy y cuánto tiempo estuvieron (basado en duración de RSSI alto). Detecta comportamientos anómalos (animal que no se mueve/aislado).
*   **Viabilidad:** Alta. BLE es más eficiente energéticamente que GPS para el collar.

### 🎓 Mercado Educación (Seguridad Escolar)
*   **Producto:** "Lista de Clase Automatizada y Evacuación".
*   **Escenario:** Simulacro de incendio o excursiones.
*   **Funcionamiento:** Los alumnos llevan beacons en mochilas. El profesor tiene una tablet con este escáner. En segundos, sabe quién falta ("Faltan 3 alumnos de 25").
*   **Valor:** Velocidad de reacción en emergencias.

### 🏢 Mercado Eventos (Gestión de Multitudes)
*   **Producto:** "Analizador de Flujo de Visitantes".
*   **Escenario:** Ferias y congresos grandes.
*   **Funcionamiento:** Escáneres en cada stand. Rastrean las acreditaciones inteligentes de los visitantes.
*   **Dato Vendible:** *"La empresa X tuvo 500 visitas únicas, con un tiempo promedio de permanencia de 8 minutos. El pico fue a las 11:00 AM"*. Esta data es oro para los expositores.


---
*Autor: Guía generada por Asistente Gemini bajo el rol de Arquitecto de Sistemas.*
