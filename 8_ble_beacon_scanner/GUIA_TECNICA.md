# GUÍA TÉCNICA: Beacon Scanner (Rastreador de Activos)

## 1. Introducción: Buscando Agujas en un Pajar
Un "Beacon" (Baliza) es un dispositivo simple que solo grita "¡SOY YO!" constantemente. No acepta conexiones. Son usados para localización en interiores, marketing o seguimiento de activos.

Este proyecto convierte al ESP32 en un radar sofisticado capaz de detectar, clasificar y medir la distancia (vía RSSI) de estos dispositivos.

---

## 2. Análisis del Código Crítico

### A. Estructuras de Decodificación (Ingeniería Inversa)
El estándar iBeacon de Apple no es abierto, pero es conocido. Definimos una estructura "packed" (sin relleno de bytes) para mapear los bytes crudos.
```c
typedef struct {
    uint16_t mfg_id;       // ID de Apple (0x004C)
    uint8_t  sub_type;     // Tipo (0x02)
    uint8_t  length;       // Longitud restante (0x15 = 21 bytes)
    uint8_t  proximity_uuid[16]; // UUID del grupo (ej. Museo del Prado)
    uint16_t major;        // Grupo secundario (ej. Sala Velázquez)
    uint16_t minor;        // Elemento único (ej. Cuadro Las Meninas)
    int8_t   tx_power;     // Potencia calibrada a 1 metro
} __attribute__((packed)) ibeacon_data_t;
```
**Nota:** `__attribute__((packed))` es vital. Sin ello, el compilador podría añadir bytes de relleno para alinear la memoria, rompiendo la correspondencia con los datos que vienen por aire.

### B. Endianness (El orden de los bytes)
Bluetooth transmite "Little Endian" (menor peso primero), pero los campos Major/Minor de iBeacon suelen ser "Big Endian".
```c
uint16_t major = __builtin_bswap16(beacon->major);
```
Debemos invertir los bytes antes de imprimirlos, o leeremos `0x0102` (258) como `0x0201` (513).

### C. Filtro RSSI (Umbral de Ruido)
En una oficina puede haber cientos de señales BLE.
```c
if (event->disc.rssi < -85) return 0;
```
Esto descarta cualquier señal débil (lejos o interferencia), permitiéndonos enfocarnos solo en dispositivos cercanos (< 5-10 metros).

---

## 3. Flujo de Eventos

1.  **Recepción:** NimBLE recibe un paquete de anuncio.
2.  **Filtro 1 (RSSI):** ¿Es fuerte? Si no, adiós.
3.  **Parsing:** NimBLE extrae el campo "Manufacturer Data".
4.  **Filtro 2 (ID):** ¿Empieza por `0x4C00` (Apple)? Si no, ignorar.
5.  **Filtro 3 (Estructura):** ¿Tiene la longitud y subtipos correctos?
6.  **Decodificación:** Mapeamos los bytes a la estructura `ibeacon_data_t`.
7.  **Salida:** Mostramos "iBeacon Detectado: Major 10, Minor 20".

---

## 4. Estudio de Mercado y Viabilidad de Producto

El escáner de Beacons es la pieza central de la **Infraestructura IoT**. Transforma señales mudas en datos de localización e identidad.

### 🏢 Mercado Corporativo (Control de Asistencia)
*   **Producto:** "Reloj Checador Invisible".
*   **Problema:** Los empleados olvidan fichar o pierden tarjetas.
*   **Solución:** Instalar este escáner en la entrada. Detecta los Beacons que los empleados llevan en sus gafetes (o la señal BLE de la App corporativa en sus móviles).
*   **Valor:** Fichaje automático "Hands-free". Genera reportes de puntualidad y ocupación de oficinas sin fricción.

### 🚚 Mercado Logística (Cross-Docking)
*   **Producto:** "Arco de Lectura de Palets".
*   **Escenario:** Puertas de carga de camiones.
*   **Funcionamiento:** Un array de antenas direccionales con ESP32. Al pasar un montacargas con un palet etiquetado, el sistema detecta qué palet entró en qué camión.
*   **Viabilidad:** Mucho más barato que RFID UHF para volúmenes medios, y permite usar sensores activos (ej. palet que también mide temperatura).

### 🏥 Mercado Hospitalario (Trazabilidad de Pacientes)
*   **Producto:** "Sistema de Protección de Errantes".
*   **Aplicación:** Pacientes con Alzheimer o demencia senil.
*   **Funcionamiento:** El paciente lleva una pulsera iBeacon sellada. Escáneres en las puertas de salida. Si el escáner detecta la pulsera con RSSI alto (cerca de la puerta) y la puerta se abre, bloquea la salida y alerta a enfermería.
*   **Valor:** Seguridad del paciente y reducción de responsabilidad legal para el hospital.


---
*Autor: Guía generada por Asistente Gemini bajo el rol de Arquitecto de Sistemas.*
