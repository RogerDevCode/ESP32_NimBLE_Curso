# Maestría en ESP32 y BLE: De "Hola Mundo" a Gateway IoT Industrial
*Una guía para ingenieros que buscan robustez, no solo prototipos.*

---

## Introducción: La Diferencia entre "Funciona" y "Producción"

Bienvenido. Si estás leyendo esto, probablemente ya has encendido un LED con Arduino. Eso está bien para empezar, pero en el mundo real, en la industria, "funcionar a veces" es inaceptable.

Como Arquitecto de Soluciones IoT, he visto demasiados proyectos fallar porque se construyeron sobre cimientos de juguete. Este tutorial no es solo una lista de instrucciones; es una inmersión en la ingeniería de software para sistemas embebidos. Usaremos **ESP-IDF**, **CMake** y **NimBLE**. ¿Por qué? Porque cuando tu dispositivo esté desplegado en 1000 ubicaciones remotas, agradecerás la estabilidad y eficiencia que estas herramientas profesionales ofrecen.

Prepárate. Vamos a construir un sistema de tracking de proximidad, pero lo haremos con **precisión quirúrgica**.

---

## Fase 1: Los Cimientos (Proyectos 1-3)

### 1. El Entorno Profesional (Hola Mundo)
Olvídate del IDE de Arduino por un momento. En el **Proyecto 1**, configuramos CMake.
*   **¿Por qué nos importa?** CMake te da control total sobre la compilación. En producción, necesitas saber exactamente qué librerías se incluyen y cómo se optimiza tu código.
*   **Nota del Arquitecto:** Un parpadeo de LED aquí no es trivial. Estamos validando que tu toolchain es capaz de flashear un binario firmado y optimizado. Si esto falla, nada más importa.

### 2. Gritando al Vacío (Advertising)
En el **Proyecto 2**, tu ESP32 se convierte en un faro.
*   **El Concepto:** El "Advertising" es como una estación de radio. Transmites, pero no sabes quién escucha.
*   **Eficiencia:** Un error común es transmitir demasiado rápido. ¿Realmente necesitas anunciar tu presencia cada 20ms? Eso drena la batería. En este curso aprenderás a ajustar los intervalos para equilibrar visibilidad y consumo energético.

### 3. Estructura y Orden (GATT)
El **Proyecto 3** introduce el orden. GATT es tu base de datos en el aire.
*   **La Metáfora:** Imagina un archivador. El "Servicio" es el cajón, la "Característica" es la carpeta.
*   **Regla de Oro:** Usa UUIDs estándares cuando puedas, pero no temas crear los tuyos propios para datos propietarios. La organización aquí define qué tan fácil será integrar tu dispositivo con una App móvil o un Gateway.

---

## Fase 2: La Conversación (Proyectos 4-7)

### 4. El Arte de Escuchar (Scanner)
En el **Proyecto 4**, aprendemos a escuchar.
*   **El Desafío:** El aire está sucio. Hay ruido, microondas, otros dispositivos. Tu escáner debe ser capaz de filtrar la señal del ruido.
*   **Nota del Arquitecto:** No proceses todo lo que escuchas. Filtra por UUID o nombre *antes* de intentar analizar el paquete. Ahorra ciclos de CPU y memoria.

### 5. El Apretón de Manos (Conexión)
El **Proyecto 5** es donde ocurre la magia: La conexión.
*   **Realidad:** Conectarse es costoso (energéticamente y en tiempo). Mantén las conexiones cortas o usa parámetros de conexión que permitan al dispositivo dormir entre intercambios de datos.

### 6. Acción y Reacción (Escritura)
En el **Proyecto 6**, tomamos el control. Escribimos datos para cambiar el estado físico (un LED, un relé).
*   **Seguridad:** Aquí es donde un junior solo envía un "1". Un senior se pregunta: "¿Qué pasa si alguien más envía ese '1'?". Aunque en este curso nos centramos en la mecánica, ten siempre presente la seguridad.

### 7. No Preguntes, Espera (Notificaciones)
El **Proyecto 7** es mi favorito.
*   **El Error del Novato:** Polling. Preguntar "¿Ya cambiaste?" cada 100ms. Eso es ineficiente.
*   **La Solución Pro:** Notificaciones. El servidor te avisa cuando algo cambia.
*   **Filosofía:** El mejor código es el que no se ejecuta hasta que es absolutamente necesario. Las notificaciones son la encarnación de esta filosofía.

---

## Fase 3: La Aplicación Real (Proyectos 8-10)

### 8. ¿Dónde estás? (Beacons y RSSI)
En el **Proyecto 8**, usamos la física de la radio. La potencia de la señal (RSSI) nos dice qué tan lejos está algo.
*   **Advertencia:** El RSSI es volátil. Rebota en las paredes, lo absorbe tu cuerpo. No confíes en una sola lectura. Necesitas promedios, filtros y lógica robusta.

### 9. Gestión de Masas (Multi-Scanner)
El **Proyecto 9** es una prueba de estrés. ¿Qué pasa si hay 50 dispositivos alrededor?
*   **Gestión de Memoria:** Aquí es donde Arduino suele fallar y ESP-IDF brilla. Aprenderemos a gestionar listas dinámicas de dispositivos sin fragmentar la memoria RAM hasta el colapso.

### 10. El Puente al Mundo (Gateway MQTT)
El gran final. El **Proyecto 10** conecta tu red BLE local a la nube vía WiFi y MQTT.
*   **La Arquitectura:** El ESP32 actúa como traductor. Recibe BLE, empaqueta en JSON, y envía por MQTT.
*   **Manejo de Errores Industrial:**
    *   **Watchdogs:** Implementaremos Task Watchdogs (TWDT) para reiniciar tareas colgadas.
    *   **Reconexión Exponencial:** Si el WiFi cae, no reintentamos a lo loco. Usamos un algoritmo de "Exponential Backoff" para no saturar la red al volver.
    *   **Colas (Queues):** Desacoplamos la recepción BLE del envío MQTT usando FreeRTOS Queues. Si el WiFi es lento, la cola absorbe el pico. Si la cola se llena, decidimos qué tirar (política de descarte).

---

## Fase 4: Infraestructura y Optimización (Proyecto 11 y Backend)

### Configuración del Backend: MQTT y n8n
Para que el Gateway tenga sentido, necesita un destino.
1.  **Broker MQTT (Mosquitto):** El sistema nervioso central.
    *   *Setup:* Docker container local.
    *   *Seguridad:* No uses puerto 1883 abierto. Implementaremos TLS/SSL si es posible, o al menos autenticación usuario/pass robusta.
2.  **n8n (Orquestador):** Tu cerebro lógico.
    *   *Integración:* Usaremos el nodo "MQTT Trigger".
    *   *Cloudflare Tunnel:* Dado que usas un túnel, tu n8n es accesible desde fuera, pero el MQTT suele ser local. El ESP32 hablará con la IP local del broker, y n8n (corriendo localmente o conectado al broker) procesará los mensajes.
    *   *Pipeline:* `MQTT Trigger` -> `JSON Parse` -> `Batching` (agrupar lecturas) -> `Database` (InfluxDB/Postgres).

### 11. Proyecto: Hibernación Profunda (Deep Sleep)
La diferencia entre una batería que dura 2 días y una que dura 2 años.
*   **El Ciclo de Vida:** Despertar -> Medir/Anunciar -> Dormir.
*   **RTC Memory:** La RAM normal se apaga. Aprenderemos a guardar estado (contadores, flags) en la memoria RTC (Slow Memory) que sobrevive al Deep Sleep.
*   **Boot Causes:** El ESP32 necesita saber *por qué* despertó (¿Timer? ¿Botón? ¿Sensor externo?). Analizaremos `esp_sleep_get_wakeup_cause()`.
*   **Optimizaciones de Hardware:** Apagar dominios de energía de periféricos no usados (ADC, I2C) antes de dormir.

---

## ¿Listo para empezar?

No corras. Entiende cada línea de código. La diferencia entre un aficionado y un profesional es que el profesional sabe *por qué* funciona su código.

Ve a la carpeta `1_hola_mundo` y comencemos.
