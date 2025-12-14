# Configuración de Infraestructura Backend: MQTT y n8n
*Orquestación de datos IoT con precisión industrial*

---

## Introducción

Este documento detalla el despliegue de la infraestructura necesaria para recibir, procesar y visualizar los datos generados por nuestro Gateway ESP32. Utilizaremos **Mosquitto** como broker MQTT y **n8n** como orquestador de flujos, expuesto a internet mediante un túnel de Cloudflare.

## 1. Despliegue del Broker MQTT (Mosquitto)

En un entorno de producción, el broker es el corazón del sistema. Usaremos Docker para un despliegue limpio y reproducible.

### Paso 1.1: Estructura de Directorios
Crea una carpeta para tu stack IoT:
```bash
mkdir -p iot-stack/mosquitto/config
mkdir -p iot-stack/mosquitto/data
mkdir -p iot-stack/mosquitto/log
cd iot-stack
```

### Paso 1.2: Configuración de Mosquitto
Crea el archivo `mosquitto/config/mosquitto.conf`.
**Nota Crítica:** Por defecto, Mosquitto v2+ no permite conexiones externas ni anónimas. Debemos configurarlo explícitamente.

```conf
persistence true
persistence_location /mosquitto/data/
log_dest file /mosquitto/log/mosquitto.log

# Listener estándar
listener 1883
# IMPORTANTE: Para pruebas locales permitimos anónimos. 
# En producción, cambiar a 'false' y usar password_file.
allow_anonymous true
```

### Paso 1.3: Docker Compose
Crea un archivo `docker-compose.yml` en la raíz de `iot-stack`:

```yaml
version: '3.8'
services:
  mosquitto:
    image: eclipse-mosquitto:2
    container_name: mqtt_broker
    restart: unless-stopped
    ports:
      - "1883:1883" # Puerto MQTT estándar
      - "9001:9001" # WebSockets (opcional, útil para debug web)
    volumes:
      - ./mosquitto/config:/mosquitto/config
      - ./mosquitto/data:/mosquitto/data
      - ./mosquitto/log:/mosquitto/log
```

### Paso 1.4: Iniciar el Servicio
```bash
docker-compose up -d
```
Verifica que esté corriendo con `docker ps`.

---

## 2. Integración con n8n (Vía Cloudflare Tunnel)

Dado que ya tienes un túnel de Cloudflare apuntando a tu instancia local de n8n, nos centraremos en cómo conectar n8n con tu broker MQTT local.

### Arquitectura del Flujo
*   **ESP32** -> (WiFi Local) -> **Mosquitto (Puerto 1883)**
*   **n8n** -> (Red Docker/Local) -> **Mosquitto (Puerto 1883)**

**Nota:** Aunque n8n sea accesible desde fuera vía Cloudflare, la conexión MQTT ocurre dentro de tu red local (o red de Docker). El tráfico MQTT no necesita pasar por el túnel.

### Paso 2.1: Crear el Nodo "MQTT Trigger"
1.  Abre tu n8n en el navegador.
2.  Crea un nuevo Workflow.
3.  Añade el nodo **MQTT Trigger**.

### Paso 2.2: Configurar Credenciales MQTT en n8n
1.  En el nodo MQTT, busca "MQTT Connection" y selecciona "Create New".
2.  **Protocol:** `mqtt`
3.  **Host:**
    *   Si n8n corre en el mismo Docker Compose: `mosquitto`
    *   Si n8n corre en otro lado pero en la misma red: IP local de tu PC (ej: `192.168.1.X`). **No uses `localhost`** si n8n está en Docker, ya que `localhost` sería el propio contenedor de n8n.
4.  **Port:** `1883`
5.  **Topics:** `esp32/gateway/data` (o el tópico que definas en el firmware).

### Paso 2.3: Procesamiento de Datos (Pipeline)
Una vez que el nodo MQTT recibe el JSON del ESP32:
1.  **Nodo JSON Parse:** Si el ESP32 envía un string, conviértelo a objeto.
2.  **Nodo Switch/If:** Filtra lecturas erróneas o valores nulos.
3.  **Nodo Database (ej: Postgres/InfluxDB):** Guarda el dato histórico.

---

## 3. Validación del Sistema

Para probar sin el ESP32, usa otra terminal para simular un dispositivo:

```bash
# Requiere tener cliente mosquitto instalado (sudo apt install mosquitto-clients)
mosquitto_pub -h localhost -t "esp32/gateway/data" -m '{"device": "BEACON-1", "rssi": -65}'
```

Si todo es correcto, verás la ejecución verde en tu canvas de n8n instantáneamente.

---

## 4. Consideraciones de Seguridad (Producción)

1.  **Autenticación MQTT:**
    *   Entra al contenedor: `docker exec -it mqtt_broker sh`
    *   Crea usuario: `mosquitto_passwd -c /mosquitto/config/passwd usuario_esp32`
    *   Actualiza `mosquitto.conf`: `allow_anonymous false` y `password_file /mosquitto/config/passwd`.
2.  **TLS/SSL:**
    *   Si el tráfico sale de la red local, el puerto 1883 es inseguro (texto plano). Configura certificados en Mosquitto para usar el puerto 8883 con encriptación.
