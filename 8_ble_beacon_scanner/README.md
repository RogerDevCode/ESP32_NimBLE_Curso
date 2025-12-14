# 🛡️ Safety-Critical Beacon Scanner

## 1. Propósito
Este proyecto implementa un escáner BLE pasivo avanzado, diseñado para el análisis de tráfico en entornos industriales o de trazabilidad.

**Diferenciadores:**
- No solo "escucha", sino que **decodifica** e **interpreta**.
- Implementa filtros de calidad de señal (RSSI) para descartar ruido.
- Posee un monitor de salud que alerta sobre silencio de radio.

## 2. Formatos Soportados
El escáner analiza automáticamente:
1.  **iBeacon (Apple):** Decodifica UUID, Major, Minor y Tx Power.
2.  **Eddystone (Google):** Detecta la presencia del servicio `0xFEAA`.
3.  **Genéricos:** Aplica filtros de duplicados para mantener el log limpio.

## 3. Arquitectura de Seguridad
- **Stack Allocation:** Todas las estructuras de configuración residen en el Stack.
- **Buffer Safety:** Uso de `__attribute__((packed))` y validación de longitud antes de *casting*.
- **Anti-Flood:** Filtro de duplicados a nivel de controlador (`filter_duplicates = 1`) y umbral de RSSI (-85dBm).

## 4. Salida Esperada
```text
I (842) SAFE_SCANNER: Scanner started (RSSI Threshold: -85 dBm)
I (1234) SAFE_SCANNER: --- iBEACON DETECTED ---
I (1234) SAFE_SCANNER: Addr: 11:22:33:44:55:66 | RSSI: -65
I (1234) SAFE_SCANNER: UUID: 00 11 22 ... FF
I (1234) SAFE_SCANNER: Major: 10, Minor: 20, TxPwr: -59
...
I (10842) SAFE_SCANNER: Status: Scanning active. Total packets: 45
```
