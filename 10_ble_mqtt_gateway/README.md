# 🛡️ Proyecto 10: Industrial BLE to MQTT Gateway (Secure Edition)

## 🌟 La Evolución: De "Hola Mundo" a Seguridad Industrial

Este proyecto representa la culminación de una serie de aprendizajes. Comenzamos encendiendo un LED, pasamos por escaneos Bluetooth simples, y ahora tenemos un **Gateway IoT de Grado Industrial**.

**¿Qué hace diferente a este código?**
A diferencia de un script de hobby, este firmware está diseñado para **no fallar**. Asume que el WiFi se cortará, que el Broker MQTT rechazará la conexión y que el hardware podría bloquearse. Para cada uno de estos escenarios, tiene una contramedida.

### Arquitectura del Sistema
Utilizamos el patrón **Productor-Consumidor** para desacoplar procesos críticos:

1.  **Productor (NimBLE):** Escanea el espectro de radio a alta velocidad. Es una tarea de *Tiempo Real*. No puede detenerse a esperar que el WiFi conecte.
2.  **Buffer (FreeRTOS Queue):** Una "sala de espera" en RAM que almacena los paquetes temporalmente.
3.  **Consumidor (MQTT Task):** Toma datos del buffer y los envía a la nube. Si el internet es lento, no afecta al escáner.

---

## 🧠 Snippets Educativos: Los Pilares del Código

### 1. Desacoplamiento (La Cola)
En lugar de procesar todo en el callback del Bluetooth (lo cual bloquearía el sistema), simplemente enviamos el dato a una cola.

```c
// En el callback de BLE (ISR context o High Priority)
if (xQueueSend(ble_evt_queue, &msg, 0) != pdTRUE) {
    // Si la cola está llena, descartamos el paquete para proteger la memoria
    ESP_LOGW(TAG, "Queue Full - Dropping packet");
}
```

### 2. Seguridad en Configuración (Adiós #defines)
En las primeras versiones, escribíamos la clave del WiFi en el código. **Eso es un error de seguridad grave**. Ahora, leemos de una partición segura (NVS).

```c
// Lectura dinámica en tiempo de ejecución
static void load_config_from_nvs(void) {
    nvs_handle_t handle;
    // Abrimos el namespace "gateway_config"
    nvs_open("gateway_config", NVS_READONLY, &handle);
    
    // Cargamos las credenciales en variables RAM
    nvs_get_str(handle, "wifi_ssid", WIFI_SSID, &len);
    nvs_get_str(handle, "wifi_pass", WIFI_PASS, &len);
    
    nvs_close(handle);
}
```

### 3. El "Perro Guardián" (Watchdog Timer)
¿Qué pasa si una tarea entra en un bucle infinito? El Watchdog (WDT) reinicia el chip automáticamente.

```c
// En tu bucle infinito
while(1) {
    // ... hacer trabajo ...
    
    // "Alimentar al perro" para decirle al sistema que seguimos vivos
    esp_task_wdt_reset(); 
    vTaskDelay(pdMS_TO_TICKS(1000));
}
```

---

## 🧱 Deep Dive: La Tabla de Particiones (`partitions.csv`)

Una de las preguntas más comunes es: **"¿Por qué no usar la tabla por defecto?"**

### ¿Qué es?
Imagina la memoria Flash del ESP32 como un disco duro. La tabla de particiones es el índice que dice: "De aquí a aquí va el código, de aquí a aquí guardo claves, etc.".

### ¿Por qué la cambiamos en este proyecto?
1.  **Espacio para Seguridad:** Al activar **Flash Encryption**, el *Bootloader* crece de tamaño. Necesitamos mover todo hacia adelante (offset `0x10000`) para que quepa.
2.  **Organización:** Necesitábamos una partición **NVS** específica para nuestras credenciales inyectadas, separada de la partición de datos del sistema (NVS keys).
3.  **Tamaño de App:** Las aplicaciones industriales con WiFi + BT + MQTT + SSL suelen ser grandes. La tabla por defecto a veces se queda corta para la aplicación (`factory`).

### Desventajas
*   **Complejidad:** Debes calcular bien los offsets (direcciones de memoria) en hexadecimal. Si se solapan, el chip no arranca.
*   **Menos espacio de usuario:** Al reservar espacio para seguridad o NVS, te queda menos para guardar archivos (SPIFFS).

---

## 🔐 Seguridad y Rendimiento: La Verdad Incómoda

Hemos activado **Flash Encryption**. ¿Qué implica esto realmente?

### 1. ¿Mi chip se volverá lento?
**NO.** El ESP32 tiene un motor de hardware dedicado (AES Accelerator) transparente.
*   **Lectura:** Cuando la CPU pide un dato a la Flash, el hardware lo desencripta "al vuelo" antes de que llegue al caché de la CPU. El impacto es imperceptible (nanosegundos).
*   **Escritura:** Escribir en Flash sí es un poco más lento porque debe encriptar antes de guardar, pero en un Gateway rara vez escribimos en Flash durante la operación normal.

### 2. ¿Pueden copiar mi código?
Con **Flash Encryption activado**:
*   Si alguien conecta un lector externo a tu chip, solo leerá **ruido aleatorio (basura)**.
*   Si intentan copiar tu firmware a otro ESP32, **no funcionará**, porque la clave de desencriptado está guardada físicamente en los eFuses internos del chip original y nadie puede leerla.

**Conclusión:** Puedes usar todos los recursos del ESP32 (WiFi, BLE, Dual Core) sin penalización de rendimiento, pero con la tranquilidad de que tu propiedad intelectual está protegida.

---

## 🚀 Potencial y Futuro: ¿Qué más podemos hacer?

Este proyecto es la base ("esqueleto") para soluciones comerciales reales:

1.  **Control de Aforo:** Contar cuántos dispositivos únicos (personas) hay en una sala basándose en las direcciones MAC detectadas.
2.  **Asset Tracking en Hospitales:** Pegar iBeacons a equipos médicos costosos. El ESP32 reporta en qué habitación están (basado en el RSSI más fuerte).
3.  **Marketing de Proximidad:** Detectar clientes en una tienda y enviar esa data a un servidor para analítica.
4.  **Automatización Industrial:** Leer sensores BLE (temperatura/vibración en motores) y enviar alarmas por MQTT si un motor se sobrecalienta, sin cables.

---

## 📈 El Siguiente Paso Profesional: Del 'NVS Injection' al 'Dynamic Provisioning'

Hemos logrado que las credenciales no estén en el código, sino en una partición NVS cifrada. Este método, que llamamos **"NVS Injection" (o Provisionamiento Estático en Fábrica)**, es un gran avance y es perfectamente válido y robusto para escenarios específicos:

*   **Uso Ideal:** Proyectos B2B (Business-to-Business), entornos industriales con redes controladas, o flotas de dispositivos donde tú (el integrador) tienes control total sobre las credenciales de la red y las configuras una única vez antes del despliegue. Imagina una fábrica con 100 sensores: el instalador flashea las credenciales una vez en tu línea de producción, y listo.

**Pero, ¿qué pasa si quieres vender este dispositivo a un cliente final (B2C) para su hogar o una oficina que no sea la tuya?** Tú no puedes saber su clave de Wi-Fi de antemano. Aquí es donde entra el **Provisionamiento Dinámico (Dynamic Provisioning)**.

### ¿Qué es el Provisionamiento Dinámico?
Es el método que usan todos los productos IoT que compras en una tienda. El dispositivo viene "en blanco" y permite al usuario final configurarlo fácilmente a su red Wi-Fi.

**Pasos Básicos:**
1.  El ESP32 arranca y detecta que no tiene credenciales Wi-Fi válidas en su NVS.
2.  Entra en un "Modo de Configuración", que suele ser un Punto de Acceso Wi-Fi temporal o un servicio BLE.
3.  El usuario final utiliza una aplicación móvil (o un navegador web) para conectarse a este "Modo de Configuración".
4.  A través de esa interfaz, el usuario selecciona su red Wi-Fi y introduce su contraseña.
5.  El ESP32 recibe las credenciales, las guarda de forma segura en la NVS (¡como ya lo hacemos!) y se conecta a la red Wi-Fi del usuario.

### Métodos de Provisionamiento Dinámico en ESP-IDF (Estándares de la Industria):

*   **1. Wi-Fi Manager (Captive Portal / SoftAP):**
    *   **Funcionamiento:** El ESP32 se convierte en un Punto de Acceso (AP) y, al conectarse a él desde un móvil/PC, el navegador web se abre automáticamente a una página de configuración.
    *   **Ventaja:** No requiere una aplicación móvil dedicada. Compatible con cualquier dispositivo con navegador.
    *   **Escenario:** Ideal para configuración rápida y universal.

*   **2. ESP-IDF Provisioning (Unified Provisioning - BLE/SoftAP):**
    *   **Funcionamiento:** Ofrece una experiencia de usuario más fluida a través de Bluetooth Low Energy (BLE) o SoftAP. Espressif proporciona librerías para que desarrolles tu propia app iOS/Android que se conecta al ESP32.
    *   **Ventaja:** Más rápido, más elegante, y puedes personalizar la experiencia con tu marca.
    *   **Escenario:** Productos comerciales que buscan una experiencia de usuario premium.

**Conclusión:** Mientras que el "NVS Injection" es el camino a seguir para el despliegue industrial controlado, el "Dynamic Provisioning" es indispensable para llevar tu producto al mercado masivo.

---

## 💡 Corolario del Programador (Lo que nadie te dice)

Si vas a llevar esto a producción, grábate esto a fuego:

1.  **El WiFi MIENTE:** `wifi_connected = true` no significa que tengas internet. Significa que estás asociado al Router. Siempre verifica la conexión a nivel de aplicación (MQTT Ping).
2.  **La Memoria se Fragmenta:** En C, hacer `malloc` y `free` constantemente (como al crear JSONs dinámicos) fragmenta la RAM. Prefiere usar buffers estáticos (variables globales o arrays fijos) siempre que puedas.
3.  **Bloquear es Morir:** Nunca uses `delay()` o bucles `while(condicion)` dentro de un callback de red o Bluetooth. Si bloqueas, el Watchdog te reiniciará. Usa Colas (`Queues`) y Semáforos.
4.  **Logs con Medida:** Los `ESP_LOGI` son geniales para desarrollar, pero lentos. En producción, baja el nivel a `ESP_LOGW` (Warnings) o `ESP_LOGE` (Errores) para ganar velocidad.
5.  **Provisionamiento:** Nunca entregues al cliente un dispositivo con tu WiFi hardcodeado. Usa Bluetooth o un Punto de Acceso temporal (Captive Portal) para que el usuario configure sus propias credenciales. (Este proyecto usa NVS Injection, que es el paso previo profesional).

---
*Desarrollado con ❤️ y Paranoia Constructiva para el Curso ESP32 NimBLE.*