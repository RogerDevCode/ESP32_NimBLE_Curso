# 🦅 Maestría en Ingeniería Embebida: La Saga del ESP32 & NimBLE
*De la chispa del LED a la Fortaleza Industrial Cifrada.*

---

## 🏛️ Introducción: Abandonando el Juguete

Bienvenido al umbral donde el *hobby* termina y la **Ingeniería** comienza.

Muchos inician su viaje en el ecosistema Arduino: rápido, fácil, indulgente. Pero el mundo industrial no perdona. En una fábrica ruidosa, con redes inestables y atacantes potenciales, un `delay(1000)` es un crimen y una clave WiFi hardcodeada es una sentencia de muerte.

Este repositorio no es un simple tutorial. Es una **bitácora de evolución técnica**. A través de 11 proyectos, transformamos un simple microcontrolador en un **Gateway IoT Industrial Seguro**, capaz de sobrevivir en entornos hostiles, autogestionar su energía y proteger sus secretos criptográficos.

¿Estás listo para dejar atrás las "soluciones que funcionan a veces" y construir sistemas que **no pueden fallar**?

---

## 🧬 Fase I: La Génesis (NimBLE & Arquitectura)

Antes de correr, aprendimos a respirar. Elegimos **NimBLE** sobre Bluedroid no por capricho, sino por supervivencia: consume el 50% menos de RAM.

### 1. El Faro en la Oscuridad (Advertising)
Nuestro primer acto fue gritarle al vacío. Convertimos el ESP32 en un **Beacon**.
*   **La Lección:** No se trata solo de transmitir datos; se trata de diseñar el paquete. Aprendimos a estructurar *Manufacturer Data* para que nuestro mensaje sea único en un océano de ruido radioeléctrico.

### 2. El Arte de Escuchar (Scanner & GATT)
Luego, aprendimos a escuchar. Pero escuchar *todo* satura la CPU.
*   **Filosofía:** "Filtrar temprano, procesar tarde". Implementamos filtros de UUID para ignorar el ruido y centrarnos solo en nuestros dispositivos. Entendimos la estructura jerárquica de **GATT**: Servicios (Carpetas) y Características (Archivos).

### 3. La Conversación Asíncrona (Notify vs Polling)
Aquí cometimos el error del novato: preguntar "¿Ya?" cada 100ms.
*   **La Solución:** Cambiamos a **Notificaciones**. El dispositivo nos avisa cuando el dato cambia.
*   **Resultado:** La CPU duerme más, la radio trabaja menos, y la batería lo agradece.

---

## 🌉 Fase II: El Puente (Gateway MQTT Industrial)

El **Proyecto 10** fue nuestro punto de inflexión. El objetivo: Unir el mundo Bluetooth (Local) con la Nube (Global) vía MQTT.

### 1. Arquitectura "Trust No One" (Confía, pero verifica)
Diseñamos el sistema asumiendo el desastre:
*   **¿WiFi caído?** Implementamos **Exponential Backoff**. No martillamos el router; esperamos pacientemente (5s, 10s, 20s...).
*   **¿Broker MQTT lento?** Usamos el patrón **Productor-Consumidor** con colas FreeRTOS. El escáner BLE (Productor) nunca se bloquea; si la red es lenta, la cola absorbe el impacto o descarta inteligentemente los datos más viejos.
*   **¿Bloqueo de CPU?** Activamos el **Task Watchdog Timer (TWDT)**. Si una tarea se vuelve egoísta y no cede el control, el perro guardián reinicia el sistema para evitar un estado "zombie".

### 2. El Mito del Rendimiento vs. Seguridad
Una duda común: *"¿Si encripto mi dispositivo, se volverá lento?"*
*   **La Realidad:** El ESP32 tiene un motor **AES por Hardware**.
*   **El Veredicto:** La encriptación es transparente. Tu código corre a velocidad nativa mientras el silicio se encarga de cifrar y descifrar al vuelo. No hay excusa para no usarla.

---

## 🛡️ Fase III: La Fortaleza Digital (Seguridad)

En el mundo OT (Operational Technology), la seguridad no es un "extra". Es la base.

### 1. El Pecado Capital: Credenciales en Código
En nuestros primeros borradores, el SSID y Password vivían en `main.c`. Si subías eso a GitHub, tu red estaba comprometida.
*   **La Solución:** **NVS Injection (Provisionamiento Estático)**.
*   Creamos un script de manufactura (`provision_device.py`) que inyecta las claves en una partición binaria separada. El firmware las lee al arrancar. El código fuente permanece limpio y agnóstico.

### 2. Cifrado de Flash (Flash Encryption)
Dimos el paso definitivo. Activamos **Flash Encryption**.
*   **El Efecto:** Ahora, si alguien desuelda el chip de memoria y lo lee, solo verá ruido aleatorio. La clave de desencriptado vive en los *eFuses* del procesador, inaccesible para los mortales.
*   **El Estado:** Operamos en "Modo Desarrollo", permitiendo actualizaciones por USB, pero manteniendo la fortaleza cerrada a miradas indiscretas.

---

## ⚡ Fase IV: Inmortalidad (Deep Sleep)

El **Proyecto 11** abordó el recurso más finito: La Energía.

### 1. El Ciclo de la Vida
Entendimos que un dispositivo IoT no "corre siempre". Vive en ciclos:
1.  **Boot:** Despertar explosivo (High Power).
2.  **Work:** Medir, Conectar, Enviar (Medium Power).
3.  **Sleep:** La pequeña muerte (Ultra Low Power, ~10µA).

### 2. Recuerdos de Ultratumba (RTC Memory)
La RAM se borra al dormir. ¿Cómo recordamos cuántas veces hemos despertado?
*   Usamos `RTC_DATA_ATTR`. Una pequeña región de memoria que se mantiene viva con una batería de botón mientras el resto del universo digital se apaga.

---

## 🔮 El Futuro: Del Laboratorio al Mercado

Hemos llegado lejos, pero el horizonte se expande. ¿Cuál es el siguiente paso para convertir esto en un producto masivo (B2C)?

### El Salto: Provisionamiento Dinámico
Actualmente usamos inyección estática (genial para fábricas). Para venderle esto a un usuario final, necesitamos **Dynamic Provisioning**:
*   El dispositivo despierta "virgen".
*   Crea un Punto de Acceso (AP) temporal o usa Bluetooth.
*   El usuario, desde su App móvil, le pasa las credenciales de *su* casa de forma segura.
*   El ESP32 se configura a sí mismo y entra en servicio.

---

## 💡 Corolario del Arquitecto

Si te llevas algo de este curso, que sea esto:

1.  **La tabla de particiones no es sagrada:** Cámbiala. Necesitas espacio para NVS, para OTA, para Logs. No uses la *default* ciegamente.
2.  **El WiFi miente:** `wifi_connected = true` solo significa que el router te escuchó. No significa que tengas internet. Valida siempre la conexión extremo a extremo.
3.  **Paranoia Constructiva:** Programa asumiendo que el cable se romperá, que la memoria se llenará y que alguien intentará hackearte. Solo así construirás sistemas resilientes.
4.  **No temas al Hexadecimal:** Las direcciones de memoria (`0x9000`, `0x10000`) son solo coordenadas en el mapa de tu silicio. Aprende a leer el mapa.

---
*Escrito para la posteridad digital. Diciembre 2025.*