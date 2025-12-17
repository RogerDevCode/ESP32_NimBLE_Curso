# GUÍA TÉCNICA: Servicios y Características GATT (El Servidor de Datos)

## 1. Introducción: La Biblioteca de Datos
Si el "Advertising" (Proyecto 2) es el letrero de la tienda, **GATT (Generic Attribute Profile)** son las estanterías con productos.

En BLE, no enviamos "archivos". Exponemos variables.
*   **Servidor (Server):** El dispositivo que tiene los datos (el ESP32).
*   **Cliente (Client):** El dispositivo que quiere los datos (tu teléfono).
*   **Servicio:** Una agrupación lógica (ej. "Termostato").
*   **Característica:** Un dato específico (ej. "Temperatura Actual").

Este proyecto transforma al ESP32 en un servidor estructurado capaz de responder preguntas.

---

## 2. Análisis del Código Crítico

### A. Definición de la Estructura de Datos
En NimBLE, la estructura de la base de datos GATT se define estáticamente con arrays de estructuras. Esto es muy eficiente en memoria.

```c
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svr_svc_uuid.u, // UUID del Servicio
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = &gatt_svr_chr_uuid.u, // UUID de la Característica
                .access_cb = gatt_svr_chr_access, // <--- EL GESTOR
                .flags = BLE_GATT_CHR_F_READ, // Solo lectura
            },
            {0} // Centinela de fin de lista
        },
    },
    {0} // Centinela de fin de lista de servicios
};
```
**Análisis:**
*   **UUIDs:** Son números de 128 bits (como `0000-4A01...`). Son la dirección única global de tu dato.
*   **`access_cb` (Callback de Acceso):** Esta es la función mágica. No necesitas escribir código para "escuchar la radio". NimBLE te llamará a esta función cuando alguien pida el dato.

### B. El Callback de Acceso (`gatt_svr_chr_access`)
Aquí es donde ocurre la lógica de negocio.

```c
static int gatt_svr_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    const char *data = "Hola Mundo BLE!";

    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        // Alguien quiere leer. ¿Tenemos el dato?
        // Copiamos el string al buffer de salida (om = output memory buffer)
        int rc = os_mbuf_append(ctxt->om, data, strlen(data));
        return (rc == 0) ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }
    
    return BLE_ATT_ERR_REQ_NOT_SUPPORTED;
}
```
**Análisis:**
1.  **Evento:** La función recibe un contexto (`ctxt`).
2.  **Verificación:** `ctxt->op` nos dice si quieren LEER o ESCRIBIR.
3.  **Acción:** Si es lectura, usamos `os_mbuf_append` para llenar el sobre de respuesta.
4.  **Retorno:** Devolver `0` significa "Éxito". Cualquier otro valor es un código de error BLE que aparecerá en el teléfono del usuario.

### C. Registro en el Arranque
En `ble_app_on_sync`:
```c
ble_gatts_count_cfg(gatt_svr_svcs); // Calcula memoria necesaria
ble_gatts_add_svcs(gatt_svr_svcs);  // Construye la tabla en RAM
ble_gatts_start();                  // Abre la tienda
```
Es vital llamar a estas funciones **antes** de empezar el advertising. Si te anuncias antes de preparar la tabla GATT, un cliente rápido podría conectarse y encontrar una base de datos vacía.

---

## 3. Flujo de Eventos (Interacción Cliente-Servidor)

1.  **Conexión:** El teléfono ve el anuncio y envía "CONNECT_REQ". El ESP32 acepta y deja de anunciar (según configuración).
2.  **Descubrimiento:** El teléfono pregunta: *"¿Qué servicios tienes?"*. El stack NimBLE responde automáticamente enviando la lista de UUIDs.
3.  **Lectura:** El usuario en el teléfono pulsa "Leer" en la característica `...4A01`.
4.  **Interrupción:** NimBLE pausa lo que esté haciendo, y ejecuta `gatt_svr_chr_access`.
5.  **Respuesta:** Tu código copia "Hola Mundo BLE!" al buffer. NimBLE encapsula eso en paquetes de radio y los envía.
6.  **Visualización:** El teléfono recibe los bytes y muestra el texto.

## 4. Pruebas y Validación
1.  Usa **nRF Connect**.
2.  Conéctate al dispositivo. Verás que el botón "CONNECT" cambia a "DISCONNECT" y se abren nuevas pestañas.
3.  Busca el servicio con UUID desconocido (el largo).
4.  Pulsa la flecha hacia abajo (⬇️) en la característica interna.
5.  Verás el valor en Hexadecimal (`48 6F ...`). Cambia la vista a **UTF-8** para leer el mensaje secreto.

---
*Autor: Guía generada por Asistente Gemini bajo el rol de Arquitecto de Sistemas.*

## 5. Estudio de Mercado y Viabilidad de Producto

Transformar el ESP32 en un servidor GATT abre la puerta a la creación de **periféricos inteligentes**. Ya no es solo una baliza, es una fuente de datos consultable.

### 🏭 Mercado Industrial (Mantenimiento Predictivo)
*   **Producto:** "Varilla de Nivel de Tanques Inalámbrica".
*   **Aplicación:** Tanques de químicos donde cablear es costoso o peligroso.
*   **Funcionamiento:** El ESP32 mide el nivel por ultrasonido. Expone el valor en una Característica GATT (`Nivel: 85%`). El operario no necesita trepar al tanque; se acerca a la base, conecta con una Tablet robusta y lee el valor.
*   **Ventaja:** Seguridad laboral y reducción de costes de cableado.

### 🏠 Mercado Doméstico (Smart Garden)
*   **Producto:** "Monitor de Salud de Plantas".
*   **Aplicación:** Dispositivo que se clava en la tierra de macetas.
*   **Funcionamiento:** Sensores capacitivos de humedad y luz. El código expone un Servicio "Planta" con características: Humedad, Luz, Fertilidad.
*   **Valor:** El usuario solo conecta cuando quiere ver el estado. Al ser conexión bajo demanda (Pull), la batería dura meses.

### 🏢 Mercado Comercial (Cadena de Frío)
*   **Producto:** "Data Logger Blackbox".
*   **Aplicación:** Transporte de vacunas o alimentos congelados.
*   **Funcionamiento:** El dispositivo graba la temperatura cada 10 min en memoria interna. Al llegar a destino, el receptor conecta por BLE y descarga el histórico (leyendo una característica larga o mediante múltiples lecturas) para certificar que nunca se rompió la cadena de frío.

