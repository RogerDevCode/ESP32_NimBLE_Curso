# GUÍA TÉCNICA: Cliente BLE Conexión y Lectura

## 1. Introducción: El Apretón de Manos
Escanear es ver. Conectarse es tocar.
Este proyecto automatiza el proceso completo:
1.  Busca un dispositivo con un **UUID de Servicio específico**.
2.  Se conecta a él.
3.  Busca la **Característica específica** dentro de ese servicio.
4.  Lee el valor una vez y lo muestra.

Es el equivalente a entrar en una habitación llena de gente, buscar a la persona con la gorra roja, saludarla y preguntarle la hora.

---

## 2. Análisis del Código Crítico

### A. Filtrado Quirúrgico (Targeted Scan)
No nos conectamos a cualquiera. Buscamos una "firma".
```c
// En el evento de descubrimiento:
for (int i = 0; i < fields.num_uuids128; i++) {
    // Comparamos el UUID del anuncio con nuestro objetivo
    if (ble_uuid_cmp(&fields.uuids128[i].u, &gatt_svr_svc_uuid.u) == 0) {
        // ¡ENCONTRADO!
        ble_gap_disc_cancel(); // Deja de escanear
        ble_gap_connect(..., &event->disc.addr, ...); // Conecta
        return 0;
    }
}
```
**Nota de Seguridad:** Comparar UUIDs completos (128 bits) es más seguro que confiar en nombres ("ESP32"), que cualquiera puede falsificar.

### B. El Baile del Descubrimiento (Service Discovery)
Una vez conectados, no sabemos *dónde* están los datos (los "handles" o manejadores son dinámicos). Debemos preguntar.
1.  **`ble_gattc_disc_all_svcs`:** "¿Qué servicios tienes?".
2.  **Callback `disc_svc_cb`:** Encuentra el servicio correcto. Llama a...
3.  **`ble_gattc_disc_all_chrs`:** "¿Qué características tiene este servicio?".
4.  **Callback `disc_chr_cb`:** Encuentra la característica correcta. Guarda su `val_handle`.

### C. La Lectura
Con el `val_handle` en mano, finalmente leemos.
```c
ble_gattc_read(conn_handle, chr->val_handle, ble_gattc_read_cb, NULL);
```

---

## 3. Pruebas y Validación (Requiere 2 ESP32)
1.  **Dispositivo A (Servidor):** Carga el *Proyecto 3*.
2.  **Dispositivo B (Cliente):** Carga este *Proyecto 5*.
3.  **Acción:**
    *   Enciende A.
    *   Enciende B.
    *   Mira el log de B.
4.  **Resultado Esperado:**
    *   B dice: "Found device... Connecting..."
    *   B dice: "Service found... Char found..."
    *   B dice: "Read complete: **Hola Mundo BLE!**"

---
*Autor: Guía generada por Asistente Gemini bajo el rol de Arquitecto de Sistemas.*

## 4. Estudio de Mercado y Viabilidad de Producto

La capacidad de conectarse y extraer datos convierte al ESP32 en un **Concentrador de Datos (Data Concentrator)**.

### 🏭 Mercado Industrial (Rondas de Inspección)
*   **Producto:** "Lector Universal de Maquinaria".
*   **Problema:** Máquinas antiguas o aisladas donde no hay WiFi.
*   **Solución:** Un dispositivo portátil (tipo pistola lectora) basado en este código. El operario apunta a la máquina, el ESP32 detecta el UUID específico, se conecta, descarga logs de error, horas de uso y temperatura, y los guarda en una tarjeta SD o los sube por LoRaWAN/NB-IoT.
*   **Valor:** Digitalización de plantas antiguas sin recablear (Retrofitting).

### 🩺 Mercado Salud (Medical Hub)
*   **Producto:** "Gateway de Telemedicina para el Hogar".
*   **Concepto:** Una caja blanca en la mesita de noche de un paciente anciano.
*   **Funcionamiento:** Se conecta automáticamente al tensiómetro BLE, al glucómetro BLE y a la báscula BLE del paciente. Lee los valores cada vez que se usan y los envía al médico vía WiFi/LTE.
*   **Valor:** Monitoreo remoto de pacientes crónicos sin que ellos tengan que usar Apps complejas de smartphone. "Usar y listo".

### 🏢 Mercado Comercial (Gestión de Energía)
*   **Producto:** "Auditor Energético".
*   **Concepto:** Dispositivo que se conecta a medidores inteligentes de enchufes (Smart Plugs) distribuidos por una oficina.
*   **Funcionamiento:** Ronda secuencial. Se conecta al enchufe A, lee consumo, desconecta. Se conecta al B, lee, desconecta.
*   **Valor:** Permite auditar el consumo eléctrico real sin depender de la nube propietaria de cada fabricante de enchufes.

