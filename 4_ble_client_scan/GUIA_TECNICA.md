# GUÍA TÉCNICA: Cliente BLE Escáner

## 1. Introducción: El Ojo que Todo lo Ve
Hasta ahora hemos sido la "tienda" (Servidor). Ahora vamos a ser el "cliente" (el comprador). El primer paso de cualquier cliente BLE es saber **quién está ahí fuera**.

Este proyecto no se conecta. Simplemente escucha el espectro radioeléctrico en busca de paquetes de anuncio, los decodifica y muestra su contenido. Es la base de un rastreador (tracker) o un gateway.

---

## 2. Análisis del Código Crítico

### A. Configuración del Escaneo
No se trata solo de decir "escanea". Debemos decir **cómo**.
```c
static void ble_app_scan(void) {
    struct ble_gap_disc_params disc_params = {0};
    
    // Filtro de duplicados: SI.
    // Si un dispositivo se anuncia cada 100ms, no queremos 10 logs por segundo.
    // Solo nos avisará la primera vez (o cuando cambie el payload).
    disc_params.filter_duplicates = 1;
    
    // Escaneo Pasivo (0) vs Activo (1)
    // Pasivo: Solo escucha.
    // Activo: Escucha y luego pregunta "Scan Request" para obtener el nombre completo.
    disc_params.passive = 1; 

    ble_gap_disc(BLE_OWN_ADDR_PUBLIC, BLE_HS_FOREVER, &disc_params, ble_gap_event, NULL);
}
```

### B. El Evento de Descubrimiento
El callback `ble_gap_event` se inunda de eventos tipo `BLE_GAP_EVENT_DISC`.
```c
case BLE_GAP_EVENT_DISC:
    // Parseamos los bytes crudos a una estructura amigable
    rc = ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data);
    
    print_adv_fields(&fields); // Función auxiliar para imprimir bonito
    
    // ¿Es conectable?
    if (event->disc.event_type == BLE_HCI_ADV_RPT_EVTYPE_ADV_IND) {
        // Podemos conectarnos a este
    }
    return 0;
```
**Análisis:** Los datos vienen en crudo (raw bytes). NimBLE ofrece `ble_hs_adv_parse_fields` que hace el trabajo sucio de separar Flags, Nombre, UUIDs y Datos de Fabricante.

---

## 3. Flujo de Eventos

1.  **Inicio:** El dispositivo arranca e inicia el escaneo infinito (`BLE_HS_FOREVER`).
2.  **Detección:** La radio detecta un paquete en el canal 37.
3.  **Filtrado:** El controlador verifica si ya vimos esta MAC recientemente. Si `filter_duplicates = 1`, lo descarta silenciosamente.
4.  **Reporte:** Si es nuevo, sube el evento al Host.
5.  **Procesado:** Tu código imprime la dirección MAC, el RSSI (potencia) y el nombre si lo tiene.

---
*Autor: Guía generada por Asistente Gemini bajo el rol de Arquitecto de Sistemas.*

## 4. Estudio de Mercado y Viabilidad de Producto

El código de escaneo pasivo es una herramienta de **Inteligencia de Negocios** y **Seguridad** extremadamente potente.

### 🏢 Mercado Comercial (Analytics & Retail)
*   **Producto:** "Contador de Aforo y Mapa de Calor".
*   **Concepto:** Colocar estos escáneres en techos de centros comerciales. Aunque no se conecten, detectan las direcciones MAC de los teléfonos (incluso aleatorizadas, sirven para conteo bruto) o dispositivos Bluetooth activos.
*   **Valor:** Generar mapas de calor: *"El 80% de los clientes se detienen 5 minutos frente a la vitrina de ropa deportiva, pero solo 10 segundos en ferretería"*. Estos datos se venden caros a los gerentes de tienda.

### 🏠 Mercado Doméstico (Seguridad Perimetral)
*   **Producto:** "Detector de Merodeadores".
*   **Concepto:** Un dispositivo en el jardín que escanea constantemente. Si detecta un dispositivo BLE desconocido con RSSI alto (muy cerca) durante más de 2 minutos a las 3 AM, asume que hay alguien con un teléfono o reloj inteligente cerca de tu ventana y enciende las luces exteriores o envía una alerta.
*   **Diferenciador:** Detecta intrusos antes de que rompan una ventana, basándose en sus emisiones digitales.

### 🏭 Mercado Industrial (Safety)
*   **Producto:** "Detector de Colisión Montacargas-Peatón".
*   **Concepto:** El montacargas lleva este escáner. Los operarios llevan chalecos con Beacons.
*   **Funcionamiento:** Si el escáner del montacargas detecta un Beacon con RSSI > -50dBm (peligro inminente), corta automáticamente la tracción del vehículo o suena una sirena potente.
*   **Valor:** Salva vidas y reduce primas de seguros.

