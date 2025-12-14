# Curso Progresivo: ESP32, BLE con NimBLE y CMake
*De "Hola Mundo" a un Gateway de Tracking de Proximidad*

---

## Introducción

Bienvenido a este curso práctico y progresivo. El objetivo es dominar el desarrollo de aplicaciones Bluetooth Low Energy (BLE) en el ESP32 utilizando el stack **NimBLE** y el sistema de compilación **CMake** con **ESP-IDF**.

A lo largo de 10 proyectos, construiremos un sistema completo de tracking de proximidad en interiores. Empezaremos con lo más básico (hacer parpadear un LED) y terminaremos con un gateway funcional que conecta múltiples dispositivos BLE a una red WiFi.

**Por qué NimBLE:** Es un stack BLE de código abierto conocido por su bajo consumo de memoria y su flexibilidad, lo que lo hace ideal para proyectos complejos en el ESP32.

**Por qué CMake y ESP-IDF:** Es el entorno de desarrollo profesional para el ESP32. Dominarlo te permitirá crear proyectos robustos, mantenibles y optimizados, más allá del ecosistema de Arduino.

### Estructura del Curso

Cada proyecto se encuentra en su propia carpeta numerada. Dentro de cada carpeta, encontrarás el código fuente y las instrucciones necesarias. Este `README` actúa como el índice y la guía principal, explicando la transición de un proyecto al siguiente.

| Paso | Proyecto | Objetivo Principal | Conceptos Clave |
|:----:|:----------|:-------------------|:----------------|
| 1 | **[Hola Mundo con CMake](./1_hola_mundo/)** | Configurar el entorno y compilar un programa básico. | ESP-IDF, CMake, Tareas de FreeRTOS, GPIO. |
| 2 | **Servidor BLE Básico (Advertising)** | Anunciar el dispositivo al mundo. | GAP, Advertising, Rol de Broadcaster. |
| 3 | **Servidor BLE con Características** | Estructurar datos para que otros los lean. | GATT, Servicios, Características, UUIDs. |
| 4 | **Cliente BLE Básico (Escaneo)** | Descubrir otros dispositivos BLE. | Escaneo Activo vs. Pasivo, Rol de Observer. |
| 5 | **Conexión y Lectura de Datos** | Conectar y leer información de un servidor. | Rol Central, Conexión, Lectura de Atributos. |
| 6 | **Escritura de Datos** | Modificar datos en un servidor de forma remota. | Escritura de Atributos (con y sin respuesta). |
| 7 | **Notificaciones y Suscripciones** | Recibir datos automáticamente del servidor. | Notificaciones, Indicaciones, CCCD. |
| 8 | **Beacons y Proximidad (RSSI)** | Estimar la distancia a un dispositivo. | Beacons, RSSI, Publicidad no conectable. |
| 9 | **Escáner Multi-Dispositivo** | Gestionar una lista de varios beacons. | Gestión de memoria, Estructuras de datos. |
| 10 | **Gateway BLE a WiFi (MQTT)** | Conectar la red BLE a Internet. | WiFi, MQTT, Coexistencia RF, Tareas concurrentes. |

---

## 1. Proyecto: "Hola Mundo" con CMake

El objetivo de este primer paso es asegurar que tienes el entorno de desarrollo ESP-IDF correctamente configurado y que puedes compilar, flashear y monitorizar un programa básico en tu ESP32-WROOM.

### 📜 Archivos del Proyecto
```
1_hola_mundo/
├── CMakeLists.txt         # Principal del proyecto
└── main/
    ├── CMakeLists.txt     # Del componente 'main'
    └── main.c             # Nuestro código fuente
```

### 🎯 ¿Qué hace el código?
El programa inicializa el microcontrolador, configura un pin GPIO (normalmente el pin 2, donde muchos DevKits tienen un LED integrado) como salida y crea una tarea de FreeRTOS que cambia el estado de ese pin (encendido/apagado) cada 500 milisegundos.

### ⚙️ ¿Cómo se hace funcionar?

**Requisito Previo:** Debes tener el framework **ESP-IDF** de Espressif instalado y los comandos `idf.py` disponibles en tu terminal.

1.  **Navega al directorio del proyecto:**
    ```bash
    cd 1_hola_mundo
    ```

2.  **Configura el proyecto:**
    La primera vez, es útil limpiar configuraciones anteriores.
    ```bash
    idf.py fullclean
    ```

3.  **Establece el chip destino (sólo la primera vez):**
    ```bash
    idf.py set-target esp32
    ```

4.  **Compila el proyecto:**
    ```bash
    idf.py build
    ```

5.  **Flashea el programa al ESP32:**
    Conecta tu ESP32 por USB. `idf.py` debería encontrar el puerto automáticamente.
    ```bash
    idf.py flash
    ```

6.  **Monitoriza la salida serial:**
    Para ver los mensajes que imprimimos por consola.
    ```bash
    idf.py monitor
    ```
    (Para salir del monitor, presiona `Ctrl+]`).

### ✅ ¿Qué se espera?
- El LED integrado en tu placa ESP32-WROOM parpadeará de forma constante (medio segundo encendido, medio segundo apagado).
- En la consola del monitor, verás un mensaje que dice "Iniciando parpadeo del LED..." seguido de "Encendiendo LED" y "Apagando LED" cada vez que cambia de estado.

### 🚨 Situaciones Extremas y Soluciones
*   **Problema:** El código compila pero el LED no parpadea.
    *   **Causa Posible:** Tu placa no tiene el LED en el pin 2.
    *   **Solución:** Busca el pin correcto para el LED de tu placa (suele ser el 2 o el 4) y modifica la línea `#define BLINK_GPIO 2` en `main.c`. Si no tienes LED integrado, puedes conectar uno externo con una resistencia de 220-330 ohms.

*   **Problema:** `idf.py` falla con el mensaje "command not found".
    *   **Causa Posible:** El entorno de ESP-IDF no está "activado" en tu terminal.
    *   **Solución:** Debes ejecutar el script de exportación de ESP-IDF. Dependiendo de tu instalación, será algo como `. $HOME/esp/esp-idf/export.sh`.

*   **Problema:** Falla el flasheo con un error de "permission denied" en Linux.
    *   **Causa Posible:** Tu usuario no tiene permisos para acceder al puerto serie `/dev/ttyUSB0`.
    *   **Solución:** Añade tu usuario al grupo `dialout` con el comando `sudo usermod -a -G dialout $USER` y reinicia la sesión.

---

## 2. Proyecto: Servidor BLE Básico (Advertising)

Ahora entramos en el mundo del BLE. El primer paso es hacer que nuestro ESP32 sea "visible" para otros dispositivos. Logramos esto a través del "advertising" (anuncio). El ESP32 emitirá paquetes de radio a intervalos regulares diciendo "¡Estoy aquí!".

### 🔄 Diferencias con el Proyecto 1 (`diff`)

*   **Archivos Nuevos:** Ninguno.
*   **Archivos Modificados:** `main/CMakeLists.txt` y `main/main.c`.

#### `main/CMakeLists.txt`
Se añade la dependencia `NimBLE` para poder usar el stack de Bluetooth.
```diff
 idf_component_register(SRCS "main.c"
                     INCLUDE_DIRS "."
-                    REQUIRES "freertos")
+                    REQUIRES "freertos" "NimBLE")
```

#### `main/main.c`
El código se reestructura completamente para inicializar el stack NimBLE y comenzar el proceso de advertising. Desaparece la lógica del parpadeo del LED.

```c
/* Includes del proyecto anterior eliminados o reemplazados por los de NimBLE */
#include "nvs_flash.h"
#include "host/ble_hs.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

// ... (nuevo código de funciones y callbacks de BLE) ...

void app_main(void)
{
    // Se elimina el código de configuración de GPIO y creación de la tarea del LED.
    // Se añade la inicialización de NVS y del stack NimBLE.
    nvs_flash_init();
    nimble_port_init();

    ble_hs_cfg.sync_cb = ble_app_on_sync;
    
    ble_svc_gap_device_name_set("ESP32-NimBLE");

    nimble_port_freertos_init(ble_host_task, NULL);
}
```

### Diagrama de Funcionamiento: Rol de Broadcaster

En este proyecto, nuestro ESP32 actúa como un "Broadcaster". No espera conexiones, simplemente emite su presencia.

```
      +-------------+
      |             |
      | ESP32-NimBLE| ----> Paquete de Anuncio (¡Estoy aquí! Soy ESP32-NimBLE)
      | (Broadcaster)|
      |             |
      +-------------+
                      \
                       \_________
                                |
                         +--------------+       +--------------+
                         |              |       |              |
                         | Smartphone   |       | Otro ESP32   |
                         | (Scanner)    |       | (Scanner)    |
                         |              |       |              |
                         +--------------+       +--------------+
```

### ⚙️ ¿Cómo se hace funcionar?
Los comandos son los mismos que en el proyecto anterior, pero aplicados a la nueva carpeta.

1.  **Navega al directorio del proyecto:**
    ```bash
    cd 2_ble_advertising
    ```
2.  **Compila, flashea y monitoriza:**
    ```bash
    idf.py flash monitor
    ```

### ✅ ¿Qué se espera?
- **No verás parpadear el LED.**
- **En la consola del monitor**, verás los logs de inicialización de NimBLE, y finalmente un mensaje que dice: `Advertising iniciado con el nombre: ESP32-NimBLE`.
- **En tu smartphone:**
    1.  Descarga una aplicación genérica para escaneo de BLE (como "nRF Connect for Mobile" o "LightBlue").
    2.  Abre la app y comienza a escanear.
    3.  Deberías ver un nuevo dispositivo en la lista llamado **"ESP32-NimBLE"**.

### 🚨 Situaciones Extremas y Soluciones
*   **Problema:** El programa se reinicia con un error `NVS_NO_FREE_PAGES`.
    *   **Causa Posible:** La partición de NVS está corrupta o es de una versión anterior de ESP-IDF.
    *   **Solución:** El código ya incluye una auto-corrección que borra y reinicia NVS si encuentra este error. Simplemente deja que se reinicie una vez.

*   **Problema:** Ves los logs en el monitor pero no encuentras el dispositivo en tu smartphone.
    *   **Causa Posible 1:** El Bluetooth de tu teléfono está apagado o la app no tiene permisos.
    *   **Solución 1:** Verifica que el Bluetooth esté encendido y que la app de escaneo tenga permisos de localización (muchas apps de BLE lo requieren).
    *   **Causa Posible 2:** Estás demasiado lejos del ESP32.
    *   **Solución 2:** Acerca tu teléfono al ESP32 (a menos de 5 metros para la primera prueba).

---

## 3. Proyecto: Servidor BLE con Servicios y Características

Si el "advertising" es la "tarjeta de presentación" de un dispositivo BLE, los **Servicios** y **Características** GATT son los "contenidos" o "archivos" que ofrece. Ahora, nuestro ESP32 no solo dirá "estoy aquí", sino que también permitirá que otros dispositivos, una vez conectados, lean información específica de él.

### 🔄 Diferencias con el Proyecto 2 (`diff`)

#### `main/main.c`
Se añade la lógica para definir y registrar un servicio GATT con una característica de solo lectura.

```c
/* --- NUEVO: Includes para servicios GAP y GATT --- */
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

/* --- NUEVO: Definición de UUIDs, callbacks y estructuras de servicio/característica --- */
// (UUIDs para identificar el nuevo servicio y la nueva característica)

static int gatt_svr_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg);

static const struct ble_gatt_chr_def gatt_svr_chrs[] = {
    // ... definición de la característica de lectura ...
};

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    // ... definición del servicio que contiene la característica ...
};

/* --- MODIFICADO: El callback on_sync ahora registra los servicios --- */
void ble_app_on_sync(void)
{
    int rc;
    ESP_LOGI(TAG, "BLE Host sincronizado.");

    // --- NUEVO: Registrar los servicios GATT definidos ---
    rc = ble_gatts_register_svcs(gatt_svr_svcs, NULL, NULL);
    assert(rc == 0);

    start_advertising();
}
```

### Diagrama de Funcionamiento: Estructura GATT

GATT (Generic Attribute Profile) organiza los datos de forma jerárquica. Nuestro dispositivo ahora es un "Servidor GATT".

```
+------------------------------------+
| Servidor GATT (ESP32)              |
|                                    |
|   +-- Servicio de Información -----+ <-- UUID del Servicio (Contenedor)
|   |                                |
|   |   +-- Característica: Estado --+ <-- UUID de la Característica (Dato)
|   |   |   (Leíble)                 |
|   |   |                            |
|   |   |   Valor: "Hola Mundo BLE!" |
|   |   +----------------------------+
|   |                                |
|   +--------------------------------+
|                                    |
+------------------------------------+
```

### ⚙️ ¿Cómo se hace funcionar?

1.  **Navega al directorio del proyecto:**
    ```bash
    cd 3_ble_services_chars
    ```
2.  **Compila, flashea y monitoriza:**
    ```bash
    idf.py flash monitor
    ```

### ✅ ¿Qué se espera?
- **En la consola del monitor**, verás logs similares al proyecto 2, pero el nombre del dispositivo ahora será **"ESP32-GATT-Svr"**.
- **En tu smartphone (usando nRF Connect for Mobile):**
    1.  Escanea y encuentra el dispositivo "ESP32-GATT-Svr".
    2.  Toca el botón **"Connect"**.
    3.  Una vez conectado, la app mostrará los servicios que ofrece el dispositivo. Verás un servicio "Unknown Service" con un UUID largo (el nuestro).
    4.  Despliega el servicio. Verás una "Unknown Characteristic" dentro.
    5.  Toca el icono de la flecha hacia abajo (`Read`).
    6.  La app debería mostrar el valor leído: **"Hola Mundo BLE!"** en formato de texto.

### 🚨 Situaciones Extremas y Soluciones
*   **Problema:** Me conecto, pero no veo ningún servicio personalizado, solo "Generic Access" y "Generic Attribute".
    *   **Causa Posible:** El registro de servicios (`ble_gatts_register_svcs`) falló o no se ejecutó.
    *   **Solución:** Revisa los logs del monitor en busca de errores después del mensaje "BLE Host sincronizado.". Asegúrate de que el `assert(rc == 0)` no esté fallando.

*   **Problema:** Intento leer la característica, pero obtengo un error en la app (ej: "Read failed").
    *   **Causa Posible:** La función de callback `gatt_svr_chr_access` tiene un error y no está poblando correctamente el buffer de respuesta.
    *   **Solución:** Revisa la implementación del callback. Asegúrate de que `os_mbuf_append` está siendo llamado correctamente y retorna 0. Puedes añadir logs dentro del callback para ver si se está ejecutando cuando intentas leer.

---

## 4. Proyecto: Cliente BLE Básico (Escaneo)

Hasta ahora, hemos creado servidores. Ahora le toca el turno al **cliente**. Este segundo ESP32 no se anunciará, sino que escuchará los anuncios de otros. Es el primer paso para que nuestros dispositivos se comuniquen entre sí.

### 🔄 Diferencias con el Proyecto 3 (`diff`)

Este proyecto es conceptualmente una rama del **Proyecto 2**. No define servicios, sino que busca los de otros.

#### `main/main.c`
El código se enfoca en la función `ble_gap_disc` y en manejar el evento `BLE_GAP_EVENT_DISC`.

```c
/* --- NO HAY SERVICIOS GATT DEFINIDOS --- */
// (Se elimina todo el código de gatt_svr_svcs, gatt_svr_chrs, y el callback de acceso)

/* --- NUEVO: Callback para manejar eventos GAP --- */
static int ble_app_gap_event(struct ble_gap_event *event, void *arg);

/* --- NUEVA: Función para iniciar el escaneo --- */
static void ble_app_scan(void)
{
    // ... configuración de parámetros de escaneo ...
    ble_gap_disc(BLE_OWN_ADDR_PUBLIC, BLE_HS_FOREVER, &disc_params, ble_app_gap_event, NULL);
}

/* --- MODIFICADO: El callback on_sync ahora inicia el escaneo --- */
void ble_app_on_sync(void)
{
    ESP_LOGI(TAG, "BLE Host sincronizado.");
    ble_app_scan(); // En lugar de start_advertising()
}
```

### Diagrama de Funcionamiento: Rol de Scanner/Observer

Ahora tenemos dos ESP32 con roles diferentes.

```
+-------------------+      (Escucha anuncios)      +--------------------+
|                   | <-------------------------- |                    |
|   ESP32 Cliente   |                             |   ESP32 Servidor   |
|     (Scanner)     |                             |  (Proyecto 2 o 3)  |
|                   |                             |    (Broadcaster)   |
+-------------------+                             +--------------------+
        |
        |
        v
+-------------------+
|  Consola/Monitor  |
| (Muestra disp.)   |
+-------------------+
```

### ⚙️ ¿Cómo se hace funcionar?

**¡IMPORTANTE!** Para este proyecto necesitas **dos placas ESP32**.

1.  **ESP32 Servidor:** Mantén flasheado y funcionando el **Proyecto 3 ("ESP32-GATT-Svr")** en una de tus placas.
2.  **ESP32 Cliente:**
    *   Conecta la segunda placa ESP32 a tu PC.
    *   Navega al directorio del proyecto: `cd 4_ble_client_scan`.
    *   Compila y flashea. Si tienes dos puertos USB, `idf.py` puede pedirte que especifiques el puerto con `-p /dev/ttyUSB1` (o el que corresponda).
        ```bash
        idf.py -p /dev/ttyUSB1 flash monitor
        ```

### ✅ ¿Qué se espera?
- En la consola del monitor del **ESP32 Cliente**, comenzarás a ver una lista de todos los dispositivos BLE que están anunciándose a tu alrededor.
- Deberías poder identificar claramente al otro ESP32 por su nombre: `Nombre: ESP32-GATT-Svr`.
- También verás su dirección MAC y el valor RSSI (la potencia de la señal, un número negativo. Ej: `-55`).

### 🚨 Situaciones Extremas y Soluciones
*   **Problema:** Mi cliente no encuentra ningún dispositivo.
    *   **Causa Posible:** El servidor (Proyecto 3) no está funcionando o está muy lejos.
    *   **Solución:** Asegúrate de que el primer ESP32 esté encendido y cerca del segundo. Verifica en tu teléfono que el "ESP32-GATT-Svr" es visible.

*   **Problema:** La consola se llena de mensajes muy rápido, es ilegible.
    *   **Causa Posible:** Estás en un entorno con muchos dispositivos BLE (oficina, departamento).
    *   **Solución:** Puedes modificar el código para que filtre y solo muestre los dispositivos que te interesan. Descomenta las líneas en `ble_app_gap_event` para que, una vez que encuentre "ESP32-GATT-Svr", detenga el escaneo con `ble_gap_disc_cancel()`.

*   **Problema:** ¿Cómo manejo dos monitores seriales para ver el cliente y el servidor al mismo tiempo?
    *   **Solución:** Abre dos ventanas de terminal. En una, ejecuta `idf.py -p /dev/ttyUSB0 monitor` para el servidor. En la otra, `idf.py -p /dev/ttyUSB1 monitor` para el cliente (ajustando los puertos). Esto es fundamental para depurar la comunicación BLE.

---

## 5. Proyecto: Conexión y Lectura de Datos

El cliente ya sabe encontrar al servidor. El siguiente paso es establecer una conexión y pedirle la información que contiene. Este es el flujo de trabajo más común en BLE: descubrir, conectar, y consumir datos.

### 🔄 Diferencias con el Proyecto 4 (`diff`)

#### `main/main.c`
La lógica principal se expande para manejar el ciclo de vida de la conexión y el descubrimiento de servicios/características.

```c
/* --- NUEVO: UUIDs del servidor que queremos encontrar --- */
// (Se añaden los UUIDs del servicio y característica del Proyecto 3)

/* --- NUEVO: Callbacks para descubrimiento y lectura --- */
// Se añaden varias funciones callback para manejar los resultados de:
// - Descubrimiento de servicios (gattc_disc_svc_cb)
// - Descubrimiento de características (gattc_disc_chr_cb)
// - Lectura de valores (gattc_read_cb)

/* --- MODIFICADO: El manejador de eventos GAP ahora gestiona la conexión --- */
static int ble_app_gap_event(struct ble_gap_event *event, void *arg) {
    switch (event->type) {
    case BLE_GAP_EVENT_DISC:
        // --- MODIFICADO: Ahora intenta conectar en lugar de solo imprimir ---
        connect_if_interesing(&event->disc);
        return 0;

    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            // --- NUEVO: Si la conexión es exitosa, descubre los servicios ---
            ble_gattc_disc_all_svcs(event->connect.conn_handle, gattc_disc_svc_cb, NULL);
        }
        return 0;
    
    // ... (manejo de desconexión)
    }
    return 0;
}
```

### Diagrama de Funcionamiento: Conexión y Lectura GATT

El proceso es una secuencia de pasos orquestada por el cliente.

```
+---------------+                               +--------------------+
|               |                               |                    |
| ESP32 Cliente |                               |   ESP32 Servidor   |
|               |                               |  (Proyecto 3)      |
+---------------+                               +--------------------+
       |                                                |
1. Escanea y encuentra al Servidor                      |
       |                                                |
2. Envía solicitud de Conexión -----------------------> |
       |                                                |
3. <----------------------- Acepta la Conexión          |
       |                                                |
4. Descubre Servicios --------------------------------> |
       |                                                |
5. <----------------------- Responde con sus Servicios |
       |                                                |
6. Descubre Características -------------------------> |
       |                                                |
7. <----------------------- Responde con sus Caracts.  |
       |                                                |
8. Solicita Leer Característica "Estado" ------------> |
       |                                                |
9. <----------------------- Envía valor: "Hola Mundo"  |
       |                                                |
       v
+-------------------+
|  Consola/Monitor  |
| "Valor: Hola..."  |
+-------------------+
```

### ⚙️ ¿Cómo se hace funcionar?
La configuración es idéntica a la del paso anterior, con dos ESP32.

1.  **ESP32 Servidor:** Mantén flasheado y funcionando el **Proyecto 3 ("ESP32-GATT-Svr")**.
2.  **ESP32 Cliente:**
    *   Navega a `cd 5_ble_client_connect_read`.
    *   Compila y flashea al segundo ESP32: `idf.py flash monitor`.

### ✅ ¿Qué se espera?
- En el monitor del **cliente**, verás los siguientes logs en secuencia:
    1.  "Escaneo iniciado."
    2.  "Servidor encontrado! Deteniendo escaneo e intentando conectar..."
    3.  "Conexión exitosa! Handle: X"
    4.  "Servicio encontrado! Descubriendo características..."
    5.  "Característica encontrada! Handle: Y"
    6.  "Lectura exitosa! Valor: **Hola Mundo BLE!**"
- Después de leer el valor, el cliente permanecerá conectado.

### 🚨 Situaciones Extremas y Soluciones
*   **Problema:** El cliente encuentra al servidor pero la conexión falla (status != 0).
    *   **Causa Posible:** Interferencia, distancia, o el servidor está ocupado.
    *   **Solución:** Acerca los dispositivos. Asegúrate de que tu teléfono no esté conectado al servidor al mismo tiempo, ya que nuestro servidor simple del Proyecto 3 solo acepta una conexión a la vez.

*   **Problema:** El cliente conecta, pero no encuentra el servicio o la característica.
    *   **Causa Posible:** Los UUIDs en el código del cliente no coinciden **exactamente** con los del servidor (Proyecto 3).
    *   **Solución:** Copia y pega las definiciones de los UUIDs desde `main.c` del Proyecto 3 a `main.c` del Proyecto 5 para asegurar que son idénticos.

*   **Problema:** El cliente se desconecta inesperadamente.
    *   **Causa Posible:** El `Connection Supervision Timeout`. Si el cliente y el servidor no intercambian paquetes dentro de un tiempo determinado (por defecto, varios segundos), la conexión se da por perdida.
    *   **Solución:** Por ahora, el código reiniciará el escaneo. En proyectos más avanzados, se puede implementar una lógica de reconexión automática.

---

## 6. Proyecto: Escritura de Datos

Ahora cerramos el círculo de comunicación básica: leer y escribir. Nuestro cliente podrá enviar datos al servidor para modificar su estado, por ejemplo, para encender un LED de forma remota.

**Nota Importante:** Este paso requiere modificar **ambos** dispositivos. Crearemos un **nuevo servidor** con una característica de escritura y un **nuevo cliente** que sepa cómo escribir en ella.

### 🔄 Diferencias con los Proyectos Anteriores (`diff`)

#### Servidor: `main.c` (Basado en Proyecto 3)
Creamos un nuevo servidor, "ESP32-Ctrl-Svr", que además de la característica de lectura, tiene una segunda característica que permite la escritura.

```c
/* --- NUEVO: GPIO para el LED a controlar --- */
#define LED_GPIO 2

/* --- NUEVO: UUID para la característica de escritura --- */
static ble_uuid128_t gatt_svr_chr_write_uuid = GATT_SVR_CHR_UUID128(GATT_SVR_CHR_UUID_BASE, 0x0002);

/* --- MODIFICADO: El array de características ahora tiene dos entradas --- */
static const struct ble_gatt_chr_def gatt_svr_chrs[] = {
    { /* Característica de Lectura (igual que antes) */ },
    {
        // --- NUEVA CARACTERÍSTICA ---
        .uuid = &gatt_svr_chr_write_uuid.u,
        .access_cb = gatt_svr_chr_access,
        .flags = BLE_GATT_CHR_F_WRITE, // <- PERMISO DE ESCRITURA
    },
    {0}
};

/* --- MODIFICADO: El callback de acceso ahora maneja la escritura --- */
static int gatt_svr_chr_access(...) {
    // ... (código de lectura igual que antes) ...

    if (ble_uuid_cmp(uuid, &gatt_svr_chr_write_uuid.u) == 0) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            // --- NUEVO: Lógica para leer el dato enviado y controlar el LED ---
            if (OS_MBUF_PKTLEN(ctxt->om) > 0) {
                uint8_t value = ctxt->om->om_data[0];
                if (value == '1') {
                    gpio_set_level(LED_GPIO, 1);
                } else if (value == '0') {
                    gpio_set_level(LED_GPIO, 0);
                }
            }
            return 0;
        }
    }
    return BLE_ATT_ERR_UNSUPPORTED_ATTR_OP;
}
```

#### Cliente: `main.c` (en la carpeta `6_ble_client_write`)
El cliente ahora busca las dos características y, una vez que las encuentra, inicia una tarea que envía '1' y '0' periódicamente.

```c
// ... (descubrimiento de servicios y características) ...

// --- MODIFICADO: El callback de descubrimiento de características ahora busca ambos UUIDs ---
static int gattc_disc_chr_cb(...) {
    // ...
    if (ble_uuid_cmp(&chr->uuid.u, &gatt_svr_chr_write_uuid.u) == 0) {
        ESP_LOGI(TAG, "Característica de ESCRITURA encontrada! Handle: %d", chr->val_handle);
        write_char_handle_global = chr->val_handle;
        
        // --- NUEVO: Iniciar una tarea para controlar el LED cuando se encuentra la característica ---
        xTaskCreate(control_led_task, "control_task", 2048, NULL, 5, NULL);
    }
    //...
}

/* --- NUEVO: Tarea que escribe periódicamente --- */
void control_led_task(void *pvParameters) {
    while(1) {
        // Escribe '1'
        ble_gattc_write_no_rsp(conn_handle_global, write_char_handle_global, "1", 1);
        vTaskDelay(pdMS_TO_TICKS(2000));
        // Escribe '0'
        ble_gattc_write_no_rsp(conn_handle_global, write_char_handle_global, "0", 1);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
```

### Diagrama de Funcionamiento: Escritura GATT

El flujo es similar a la lectura, pero en la dirección opuesta.

```
+---------------+                               +--------------------+
|               |                               |                    |
| ESP32 Cliente |                               |   ESP32 Servidor   |
| (Proyecto 6)  |                               |   (ESP32-Ctrl-Svr) |
+---------------+                               +--------------------+
       |                                                |
1. Conecta y descubre la característica de ESCRITURA.   |
       |                                                |
2. Envía comando de Escritura: '1' -------------------> |
       |                                                |
       |                              Procesa el '1' y ENCIENDE el LED.
       |                                                |
3. Espera 2 segundos.                                   |
       |                                                |
4. Envía comando de Escritura: '0' -------------------> |
       |                                                |
       |                              Procesa el '0' y APAGA el LED.
       |                                                |
       v                                                v
+-------------------+                           +--------------------+
|  Consola Cliente  |                           |  LED Físico del Srv |
| "Escribiendo: 1"  |                           |     (Parpadea)     |
+-------------------+                           +--------------------+
```

### ⚙️ ¿Cómo se hace funcionar?

1.  **ESP32 Servidor:**
    *   **IMPORTANTE:** El código de este **nuevo servidor** no está en una carpeta propia, sino que se describe en la sección `diff` de arriba. Deberás tomar el código del **Proyecto 3**, aplicarle las modificaciones del `diff` (añadir la característica de escritura y la lógica del LED), y flashearlo en tu primer ESP32. Asegúrate de que el nombre del dispositivo sea **"ESP32-Ctrl-Svr"**.
2.  **ESP32 Cliente:**
    *   Navega a `cd 6_ble_client_write`.
    *   Compila y flashea al segundo ESP32: `idf.py flash monitor`.

### ✅ ¿Qué se espera?
- El cliente escaneará, encontrará al "ESP32-Ctrl-Svr" y se conectará.
- Una vez conectado, el monitor del cliente mostrará "Escribiendo valor: 1", y dos segundos después, "Escribiendo valor: 0", y así sucesivamente.
- **En la placa del servidor**, verás el LED integrado encenderse durante 2 segundos y luego apagarse durante 2 segundos, de forma sincronizada con los mensajes del cliente.

### 🚨 Situaciones Extremas y Soluciones
*   **Problema:** El cliente conecta, pero no empieza a escribir y el LED del servidor no hace nada.
    *   **Causa Posible:** El cliente no encontró la característica de escritura. Esto suele pasar si el UUID en el cliente no coincide con el del nuevo servidor.
    *   **Solución:** Verifica que el UUID `0x0002` esté definido y buscado correctamente en ambos dispositivos. Revisa los logs del cliente para ver si aparece el mensaje "Característica de ESCRITURA encontrada!".

*   **Problema:** El cliente escribe, pero el servidor no reacciona (el LED no cambia).
    *   **Causa Posible:** La lógica en el callback de acceso `gatt_svr_chr_access` del servidor es incorrecta, o el pin del LED está mal definido.
    *   **Solución:** Añade `ESP_LOGI` dentro del `if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR)` en el servidor para confirmar que está recibiendo el evento de escritura y el valor correcto. Verifica que `LED_GPIO` corresponde al pin del LED de tu placa.

---

## 7. Proyecto: Notificaciones del Servidor

Las notificaciones son el método más eficiente para que un servidor comunique cambios a un cliente. En lugar de que el cliente pregunte (lea) repetidamente si algo ha cambiado (polling), el servidor le "avisa" proactivamente. Esto ahorra energía y reduce la latencia, siendo fundamental en BLE.

**Nota Importante:** De nuevo, este paso requiere un **nuevo servidor** (el de la carpeta `/7_ble_server_notify`) y modificar nuestro **cliente** para que se suscriba y reciba estas notificaciones.

### 🔄 Diferencias con los Proyectos Anteriores (`diff`)

#### Servidor: `main.c` (en la carpeta `7_ble_server_notify`)
Creamos un nuevo servidor "ESP32-Notify-Svr" con una característica que, en lugar de ser leída o escrita por el cliente, puede enviar notificaciones.

```c
/* --- NUEVO: UUID para la característica de notificación --- */
static ble_uuid128_t gatt_svr_chr_notify_uuid = GATT_SVR_CHR_UUID128(GATT_SVR_CHR_UUID_BASE, 0x0003);

/* --- NUEVAS VARIABLES GLOBALES --- */
static bool is_subscribed = false;
static uint16_t attr_handle_global; // Para saber a qué característica notificar

/* --- MODIFICADO: La característica ahora tiene el flag NOTIFY --- */
static const struct ble_gatt_chr_def gatt_svr_chrs[] = {
    {
        .uuid = &gatt_svr_chr_notify_uuid.u,
        .access_cb = gatt_svr_chr_access,
        .flags = BLE_GATT_CHR_F_NOTIFY, // <- PERMISO DE NOTIFICACIÓN
    },
    {0}
};

/* --- NUEVO: Tarea que envía notificaciones periódicamente --- */
void notify_task(void *pvParameters) {
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(2000));
        if (is_subscribed) {
            struct os_mbuf *om = ble_hs_mbuf_from_flat(&notify_counter, sizeof(notify_counter));
            // --- CÓDIGO CRÍTICO: Envío de notificación ---
            ble_gatts_notify_custom(conn_handle_global, attr_handle_global, om);
        }
    }
}

/* --- MODIFICADO: El callback de GAP ahora maneja la conexión/desconexión --- */
// (Para saber a quién notificar)

/* --- MODIFICADO: on_sync ahora crea la tarea de notificación --- */
void ble_app_on_sync(void) {
    // ...
    xTaskCreate(notify_task, "notify_task", 2048, NULL, 5, NULL);
}
```

#### Cliente: `main.c` (Modificaciones sobre el Proyecto 6)
El cliente debe encontrar la nueva característica y escribir en un "descriptor" especial (el CCCD) para activar las notificaciones.

```c
// --- NUEVO: Callback para manejar las notificaciones entrantes ---
static int gattc_notify_cb(uint16_t conn_handle, uint16_t attr_handle,
                           struct ble_gatt_attr *attr, void *arg) {
    ESP_LOGI(TAG, "Notificación recibida! Valor: %d", attr->om->om_data[0]);
    return 0;
}

// --- MODIFICADO: El callback de descubrimiento de características ---
static int gattc_disc_chr_cb(...) {
    // ...
    if (ble_uuid_cmp(&chr->uuid.u, &gatt_svr_chr_notify_uuid.u) == 0) {
        // --- NUEVO: Lógica para suscribirse ---
        // 1. Encontrar el descriptor CCCD (Client Characteristic Configuration Descriptor)
        // 2. Escribir el valor '0x0001' en el CCCD para habilitar notificaciones.
        // (La implementación exacta requiere descubrir descriptores y luego escribir)
        // Por simplicidad, el código final lo hará.
    }
    //...
}

/* --- MODIFICADO: El manejador de eventos GAP ahora recibe las notificaciones --- */
static int ble_app_gap_event(...) {
    switch (event->type) {
    // ...
    case BLE_GAP_EVENT_NOTIFY_RX:
        ESP_LOGI(TAG, "Notificación recibida en el evento GAP");
        // ... (procesar datos) ...
        return 0;
    // ...
    }
}
```

### Diagrama de Funcionamiento: Suscripción y Notificación

```
+---------------+                               +--------------------+
|               |                               |                    |
| ESP32 Cliente |                               |   ESP32 Servidor   |
|               |                               |    (Proyecto 7)    |
+---------------+                               +--------------------+
       |                                                |
1. Conecta y descubre la característica NOTIFY.         |
       |                                                |
2. Escribe '1' en el descriptor CCCD de la caract. ---->| (Servidor marca al cliente como "suscrito")
       |                                                |
       | <-------------------------------------------- 3. Envía Notificación (ej: contador = 1)
4. (Cliente recibe '1' y lo muestra)                    |
       |                                                |
       | <-------------------------------------------- 5. (2 seg después) Envía Notificación (contador = 2)
6. (Cliente recibe '2' y lo muestra)                    |
...y así sucesivamente...
```

### ⚙️ ¿Cómo se hace funcionar?

1.  **ESP32 Servidor:**
    *   Navega a la carpeta `7_ble_server_notify`.
    *   Compila y flashea el código en tu primer ESP32: `idf.py flash monitor`.
2.  **ESP32 Cliente:**
    *   **IMPORTANTE:** Para este paso, debes modificar el código del **Proyecto 6**. Encuentra la función `gattc_disc_chr_cb` y, además de buscar la característica de escritura, busca la de notificación (`0x0003`). Una vez encontrada, debes descubrir sus descriptores y escribir `0x0100` en el CCCD para suscribirte. Luego, en el `ble_app_gap_event`, maneja el `BLE_GAP_EVENT_NOTIFY_RX` para imprimir los datos recibidos.
    *   Flashea este código modificado en tu segundo ESP32.

### ✅ ¿Qué se espera?
- El servidor se anunciará como **"ESP32-Notify-Svr"**.
- El cliente conectará y se suscribirá.
- En el monitor del **servidor**, verás "Notificación enviada con valor: X" cada 2 segundos.
- En el monitor del **cliente**, verás "Notificación recibida! Valor: X" cada 2 segundos, de forma sincronizada con el servidor, demostrando que estás recibiendo datos sin solicitarlos.

### 🚨 Situaciones Extremas y Soluciones
*   **Problema:** El cliente conecta pero no recibe notificaciones.
    *   **Causa Posible:** La suscripción falló. El cliente no escribió correctamente en el CCCD.
    *   **Solución:** Usa una app como nRF Connect en tu teléfono. Conéctate al servidor, busca la característica de notificación y habilita las notificaciones manualmente (la app lo hace escribiendo en el CCCD por ti). Si tu teléfono recibe las notificaciones, el problema está en el código de suscripción de tu cliente. Si tu teléfono tampoco las recibe, el problema está en el servidor.

*   **Problema:** El servidor se reinicia al intentar enviar una notificación (`ble_gatts_notify_custom`).
    *   **Causa Posible:** El `conn_handle` es inválido (el cliente ya se desconectó) o el `attr_handle` de la característica es incorrecto.
    *   **Solución:** Asegúrate de que `is_subscribed` y `conn_handle_global` se resetean correctamente en el evento de desconexión. Verifica que el `attr_handle` que obtienes en el `gatt_svr_register_cb` es el correcto.

---

## 8. Proyecto: Beacons y Proximidad (RSSI)

Damos un giro para enfocarnos en el objetivo final: el sistema de tracking. En este paso, abandonamos las conexiones GATT y nos centramos en los paquetes de `advertising`. Crearemos un **Beacon**, un dispositivo que solo se anuncia sin permitir conexiones, y un **Scanner** que mide la potencia de esa señal (RSSI) para estimar la proximidad.

### 🔄 Diferencias con los Proyectos Anteriores (`diff`)

#### Beacon: `main.c` (Un nuevo servidor muy simple)
Este código es una versión simplificada del Proyecto 2. La clave es que el modo de conexión es `NON` (no conectable).

```c
// ... includes ...

void start_advertising(void)
{
    struct ble_gap_adv_params adv_params;
    // ... configuración de los campos del anuncio (nombre: "BEACON-1")...

    /* --- CÓDIGO CRÍTICO --- */
    memset(&adv_params, 0, sizeof(adv_params));
    // El beacon se anuncia de forma NO CONECTABLE.
    adv_params.conn_mode = BLE_GAP_CONN_MODE_NON;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &adv_params, NULL, NULL);
}

// ... El resto del código es una versión mínima del servidor del Proyecto 2 ...
```

#### Scanner: `main.c` (en la carpeta `8_ble_beacon_scanner`)
Este es una modificación de nuestro cliente del Proyecto 4, pero ahora se enfoca en el valor `rssi`.

```c
// --- MODIFICADO: El callback de GAP ahora se enfoca en el RSSI ---
static int ble_app_gap_event(struct ble_gap_event *event, void *arg) {
    struct ble_hs_adv_fields fields;

    switch (event->type) {
        case BLE_GAP_EVENT_DISC:
            if (ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data) == 0) {
                // Filtramos para mostrar solo nuestros beacons
                if (fields.name != NULL && strncmp((char*)fields.name, "BEACON", 6) == 0) {
                    /* --- CÓDIGO CRÍTICO --- */
                    // Imprimimos el nombre y la potencia de la señal recibida (RSSI)
                    ESP_LOGI(TAG, "Dispositivo '%.*s' encontrado, RSSI: %d",
                             fields.name_len, fields.name, event->disc.rssi);
                }
            }
            return 0;
        // ...
    }
}
```

### Diagrama de Funcionamiento: Beacon y Scanner

La comunicación es unidireccional. El Scanner es puramente un oyente.

```
+-------------+
|             |
|   BEACON-1  | -- Paquete de Anuncio -->
|             |
+-------------+
                                      \
                                       \ Señal Débil (ej: RSSI -85)
                                        \
                                         +-------------+
                                         |             |
                                         |   Scanner   |
                                         |  (Proyecto 8) |
                                         +-------------+
+-------------+
|             |
|   BEACON-1  | -- Paquete de Anuncio -->
|             |
+-------------+
      |
      | Señal Fuerte (ej: RSSI -42)
      v
+-------------+
|             |
|   Scanner   |
|  (Proyecto 8) |
+-------------+
```

### ⚙️ ¿Cómo se hace funcionar?

1.  **ESP32 Beacon:**
    *   Toma el código del **Proyecto 2**, pero modifica la sección de `start_advertising` para que el `conn_mode` sea `BLE_GAP_CONN_MODE_NON`.
    *   Cambia el nombre del dispositivo a **"BEACON-1"**.
    *   Flashea este código en tu primer ESP32.
2.  **ESP32 Scanner:**
    *   Navega a `cd 8_ble_beacon_scanner`.
    *   Compila y flashea al segundo ESP32: `idf.py flash monitor`.

### ✅ ¿Qué se espera?
- El monitor del Scanner mostrará repetidamente una línea como: `Dispositivo 'BEACON-1' encontrado, RSSI: -XX`.
- **Prueba clave:** Aleja y acerca el Beacon del Scanner. Verás que el valor de RSSI cambia. Un número más cercano a 0 (ej. de -80 a -50) significa que la señal es más fuerte y el dispositivo está más cerca.

### 🚨 Situaciones Extremas y Soluciones
*   **Problema:** El valor de RSSI fluctúa mucho aunque los dispositivos no se muevan.
    *   **Causa Posible:** Es la naturaleza de la radiofrecuencia (RF). Rebotes de la señal, obstáculos (incluido tu propio cuerpo) y la orientación de la antena afectan enormemente el RSSI.
    *   **Solución:** En un proyecto real, nunca se usa un solo valor de RSSI. Se promedian varias lecturas (ej. una media móvil de las últimas 5 lecturas) o se usa un filtro más complejo (como un filtro de Kalman) para obtener una estimación de distancia más estable.

*   **Problema:** El Scanner no detecta el Beacon.
    *   **Causa Posible:** El Beacon no se está anunciando correctamente.
    *   **Solución:** Usa tu smartphone con nRF Connect para verificar si puedes ver el dispositivo "BEACON-1". Si no lo ves, el problema está en el código del Beacon. Si lo ves, el problema está en el código del Scanner.
