# 📡 Servidor BLE con Servicios y Características

## Introducción

Imagina que tu ESP32 es una biblioteca bien organizada. En esta biblioteca, los libros (datos) no están simplemente apilados en el suelo, sino cuidadosamente ordenados en estanterías (servicios) y cajones específicos (características). Cualquier visitante (dispositivo cliente) que entre a tu biblioteca sabrá exactamente dónde buscar la información que necesita.

Este proyecto da un paso fundamental en el desarrollo con Bluetooth Low Energy: la creación de un servidor GATT (Generic Attribute Profile) estructurado que permite a otros dispositivos no solo conectarse, sino también **leer información específica** de manera organizada y profesional.

---

## 🎯 Objetivo del Proyecto

Construir un servidor BLE en el ESP32 que:

1. Se anuncia y permite conexiones de dispositivos cercanos
2. Organiza la información en **servicios** y **características**
3. Responde a solicitudes de lectura con datos personalizados
4. Implementa la arquitectura GATT de forma limpia y escalable

Este proyecto representa el tercer nivel en tu camino de aprendizaje de BLE con NimBLE, donde pasas de simplemente anunciarte a ofrecer datos estructurados y accesibles.

---

## 🏗️ Arquitectura del Sistema

### La Jerarquía GATT

El proyecto implementa una estructura jerárquica que sigue el estándar GATT:

```
Servidor BLE (ESP32-GATT-Svr)
    │
    └── Servicio de Información del Dispositivo
            │
            └── Característica: "Estado del Dispositivo"
                    └── Valor: "Hola Mundo BLE!"
```

### Componentes Principales

**1. Servicios (Services)**  
Son contenedores lógicos que agrupan características relacionadas. En este proyecto, se define un servicio personalizado identificado por un UUID único de 128 bits. Piensa en él como una categoría o sección de tu biblioteca de datos.

**2. Características (Characteristics)**  
Son los puntos de acceso reales a los datos. Cada característica tiene:
- Un **UUID único** que la identifica
- **Permisos** que definen qué operaciones se permiten (leer, escribir, notificar)
- Un **callback** que maneja las solicitudes de acceso
- Un **valor** que contiene los datos actuales

**3. UUIDs (Universally Unique Identifiers)**  
Identificadores de 128 bits que garantizan que tus servicios y características sean únicos en todo el mundo. Este proyecto usa UUIDs personalizados generados específicamente para esta aplicación.

---

## 💡 Funcionamiento Interno

### Flujo de Operación

1. **Inicialización**
   - Se inicializa la memoria flash NVS para almacenamiento persistente
   - Se inicializa el stack NimBLE
   - Se configura el nombre del dispositivo: `ESP32-GATT-Svr`

2. **Registro de Servicios**
   - Al sincronizarse el host BLE, se registran los servicios definidos
   - Se cuentan y validan las configuraciones GATT
   - Se añaden al perfil GATT del servidor

3. **Inicio del Advertising**
   - El dispositivo comienza a anunciarse como descubrible
   - Otros dispositivos BLE pueden encontrarlo y conectarse

4. **Manejo de Conexiones**
   - Cuando un cliente se conecta y lee la característica
   - Se invoca la función `gatt_svr_chr_access()`
   - Se envía el mensaje "Hola Mundo BLE!" al cliente

### El Callback de Acceso

```c
static int gatt_svr_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg)
```

Esta función es el **corazón del servidor GATT**. Cada vez que un cliente intenta leer o escribir una característica, NimBLE invoca este callback. Actualmente implementa:

- Verificación del tipo de operación (lectura)
- Copia de datos al buffer de respuesta
- Retorno de código de error apropiado si la operación no es soportada

---

## 🔧 Detalles Técnicos

### UUIDs Utilizados

El proyecto define UUIDs base personalizados:

```c
#define GATT_SVR_SVC_UUID_BASE {0x28, 0x50, 0x84, 0xde, 0x86, 0x18, 0x48, 0x94, \
                                0x93, 0x45, 0x01, 0x4a, 0x00, 0x00, 0x00, 0x00}
```

Estos identificadores únicos aseguran que tu servicio no colisione con ningún otro servicio BLE en el mundo.

### Flags de Características

```c
.flags = BLE_GATT_CHR_F_READ
```

Este flag indica que la característica es de **solo lectura**. NimBLE ofrece otros flags como:
- `BLE_GATT_CHR_F_WRITE`: Permite escritura
- `BLE_GATT_CHR_F_NOTIFY`: Permite notificaciones
- `BLE_GATT_CHR_F_INDICATE`: Permite indicaciones

---

## 🚀 Compilación y Ejecución

### Requisitos Previos

- ESP-IDF v5.5.1 o superior
- ESP32 con soporte BLE
- Cable USB para programación

### Proceso de Compilación

1. **Limpiar construcción anterior** (opcional):
   ```bash
   idf.py fullclean
   ```

2. **Compilar el proyecto**:
   ```bash
   . $HOME/esp/v5.5.1/esp-idf/export.sh
   idf.py build
   ```

3. **Flashear al ESP32**:
   ```bash
   idf.py flash
   ```

4. **Monitorear salida serial**:
   ```bash
   idf.py monitor
   ```

### Uso de Tareas VSCode

El proyecto incluye tareas preconfiguradas:
- **ESP-IDF: Build** - Compila el proyecto
- **ESP-IDF: Flash** - Flashea el firmware
- **ESP-IDF: Monitor** - Abre el monitor serial
- **ESP-IDF: Clean** - Limpia archivos de compilación

---

## 📱 Prueba del Servidor

### Usando nRF Connect (Recomendado)

1. Instala la app **nRF Connect** en tu smartphone (Android/iOS)
2. Escanea dispositivos BLE
3. Busca el dispositivo `ESP32-GATT-Svr`
4. Conéctate al dispositivo
5. Explora los servicios disponibles
6. Busca el servicio con UUID personalizado
7. Lee la característica - deberías ver: **"Hola Mundo BLE!"**

![Servicios BLE en nRF Connect](img01.jpeg)
*Captura del servidor BLE mostrando los servicios y características en nRF Connect*

### Servicios Visibles en nRF Connect

Cuando te conectes al ESP32, verás los siguientes servicios:

**Servicios Estándar (Automáticos):**
- **Generic Access (0x1800)**: Servicio estándar BLE que contiene información básica del dispositivo
  - Device Name
  - Appearance
  - Peripheral Preferred Connection Parameters

- **Generic Attribute (0x1801)**: Servicio para gestión de cambios en la tabla GATT
  - Service Changed characteristic

**Servicio Personalizado:**
- **UUID: 0000-4A01-4593-9448-1886-DE845028**: Tu servicio personalizado
  - **Característica 0x0001**: Contiene el mensaje "Hola Mundo BLE!"
  - Permisos: READ
  - Sin encriptación (OPEN)

### 📖 Cómo Leer "Hola Mundo BLE!"

1. **Conecta al dispositivo** `ESP32-GATT-Svr`
2. **Expande tu servicio personalizado** (UUID que termina en ...4A01)
3. **Busca la característica** con UUID que termina en ...0001
4. **Presiona el botón de lectura** (flecha hacia abajo ↓)
5. **Verás los datos en formato hexadecimal:**
   ```
   48 6F 6C 61 20 4D 75 6E 64 6F 20 42 4C 45 21
   ```
6. **Cambia a formato texto:** Busca el icono de conversión (generalmente `[</>]` o `[ABC]`)
7. **¡Ahora verás:** "Hola Mundo BLE!"

![Proceso de lectura paso a paso](img02.jpeg)  
*Navegación a través de los servicios*

![Característica seleccionada](img03.jpeg)  
*Propiedades y UUID de la característica*

![Valor leído](img04.jpeg)  
*Valor "Hola Mundo BLE!" en la característica*

### Usando Herramientas Linux

```bash
# Escanear dispositivos
sudo hcitool lescan

# Conectar usando gatttool
sudo gatttool -b [MAC_ADDRESS] -I
connect
characteristics
char-read-hnd [handle]
```

---

## 🎨 Posibles Usos y Aplicaciones

### Aplicaciones Inmediatas

**1. Sensor Ambiental Inteligente**
- Exponer lecturas de temperatura, humedad y presión
- Cada sensor como una característica diferente
- Aplicación móvil lee datos en tiempo real

**2. Control de Dispositivos IoT**
- Característica de lectura: estado actual del dispositivo
- Característica de escritura: comandos de control
- Perfecta para domótica

**3. Beacon de Información**
- Museo: información sobre exhibiciones
- Tienda: ofertas y promociones
- Campus: información de ubicación

**4. Monitor de Salud**
- Lectura de frecuencia cardíaca
- Pasos contados
- Calorías quemadas

### Casos de Uso Profesionales

**Industrial:**
- Monitoreo de maquinaria
- Telemetría de equipos
- Mantenimiento predictivo

**Retail:**
- Proximidad marketing
- Análisis de tráfico de clientes
- Sistemas de fidelización

**Salud:**
- Dispositivos médicos portátiles
- Monitoreo de pacientes
- Rastreo de medicación

---

## 🔮 Mejoras y Extensiones Sugeridas

### Nivel Básico

**1. Múltiples Características**
```c
// Añadir más características al servicio
- Temperatura
- Humedad  
- Nivel de batería
- Tiempo de actividad
```

**2. Características de Escritura**
```c
.flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE
```
Permitir que clientes configuren el dispositivo remotamente.

**3. Datos Dinámicos**
```c
// En lugar de "Hola Mundo BLE!" estático
- Leer datos de sensores reales
- Generar valores basados en tiempo
- Responder con información del sistema
```

### Nivel Intermedio

**4. Notificaciones**
Implementar `BLE_GATT_CHR_F_NOTIFY` para enviar actualizaciones automáticas cuando los datos cambien, sin que el cliente tenga que solicitarlos constantemente.

**5. Múltiples Servicios**
```c
- Servicio de Información del Dispositivo
- Servicio de Sensores
- Servicio de Configuración
- Servicio de Diagnóstico
```

**6. Seguridad**
Implementar encriptación y autenticación:
```c
.flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC
```

**7. Almacenamiento Persistente**
Guardar configuraciones en NVS para mantenerlas después de reinicios.

### Nivel Avanzado

**8. Descriptores de Características**
Añadir descriptores para metadata adicional:
- Client Characteristic Configuration Descriptor (CCCD)
- User Description Descriptor
- Presentation Format Descriptor

**9. Modo de Bajo Consumo**
Optimizar para dispositivos alimentados por batería:
- Deep sleep entre conexiones
- Advertising intervals optimizados
- Connection parameters ajustados

**10. OTA (Over-The-Air) Updates**
Implementar actualizaciones de firmware vía BLE:
- Servicio dedicado para OTA
- Validación de integridad
- Rollback en caso de fallo

**11. Mesh Networking**
Extender a una red BLE Mesh para:
- Comunicación multi-dispositivo
- Escalabilidad masiva
- Redundancia y confiabilidad

**12. Integración con Cloud**
Combinar BLE con WiFi:
- ESP32 como gateway BLE-to-WiFi
- Enviar datos a servicios cloud
- Control remoto vía internet

---

## 📊 Estructura del Código

```
3_ble_services_chars/
├── main/
│   ├── main.c              # Código principal del proyecto
│   └── CMakeLists.txt      # Configuración de compilación
├── build/                  # Archivos compilados (generados)
├── CMakeLists.txt          # Configuración raíz del proyecto
├── sdkconfig               # Configuración del ESP-IDF
└── README.md               # Este archivo
```

---

## 🐛 Solución de Problemas

### El dispositivo no aparece en el escaneo

- Verifica que BLE esté habilitado en tu smartphone
- Confirma que el ESP32 muestra "Advertising iniciado" en los logs
- Asegúrate de estar dentro del rango (< 10 metros sin obstáculos)

### "No services found" después de conectar

**Síntoma:** nRF Connect se conecta pero muestra "No services found"

**Causa:** Los UUIDs no están correctamente inicializados. Las macros `BLE_UUID128_INIT` deben usarse con arrays de bytes directamente.

**Solución:**
```c
// ✅ CORRECTO
static const ble_uuid128_t gatt_svr_svc_uuid = BLE_UUID128_INIT(
    0x28, 0x50, 0x84, 0xde, 0x86, 0x18, 0x48, 0x94,
    0x93, 0x45, 0x01, 0x4a, 0x01, 0x00, 0x00, 0x00
);

// ❌ INCORRECTO - No funciona
#define UUID_BASE {0x28, 0x50, ...}
static ble_uuid128_t uuid = BLE_UUID128_INIT(UUID_BASE);
```

### Error al leer la característica

- Verifica que la característica tenga el flag `BLE_GATT_CHR_F_READ`
- Confirma que el callback está correctamente implementado
- Revisa los logs para mensajes de error específicos
- Si ves datos en hexadecimal, usa el botón de formato para cambiar a texto

### La compilación falla

- Verifica la versión de ESP-IDF: `idf.py --version`
- Ejecuta `idf.py fullclean` y recompila
- Asegúrate de que todas las dependencias estén instaladas

---

## 📚 Recursos Adicionales

### Documentación Oficial

- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [NimBLE User Guide](https://mynewt.apache.org/latest/network/docs/index.html)
- [Bluetooth Core Specification](https://www.bluetooth.com/specifications/specs/)

### Herramientas Útiles

- **nRF Connect**: App móvil para testing BLE
- **LightBlue**: Alternativa para iOS
- **Wireshark**: Análisis de paquetes BLE (con hardware adecuado)
- **UUID Generator**: https://www.uuidgenerator.net/

### Comunidad

- [ESP32 Forum](https://www.esp32.com/)
- [ESP-IDF GitHub](https://github.com/espressif/esp-idf)
- [Stack Overflow - ESP32 Tag](https://stackoverflow.com/questions/tagged/esp32)

---

## 📝 Notas del Desarrollador

Este proyecto es parte de un curso progresivo de BLE con NimBLE. Representa el fundamento sobre el cual se construirán aplicaciones más complejas. La simplicidad del código actual es intencional: facilita la comprensión de los conceptos fundamentales antes de añadir complejidad.

**Principios de diseño:**
- Código claro y bien comentado
- Estructura modular y extensible
- Enfoque educativo sin sacrificar buenas prácticas
- Ejemplos prácticos y aplicables

---

## 📄 Licencia

Este proyecto es de código abierto y está disponible para uso educativo y comercial.

---

## 👨‍💻 Autor

**Roger Gallegos**  
Adaptado y documentado con asistencia de IA  
Fecha: 08-12-2025

---

## 🎓 Conclusión

Has dado un paso importante en el ecosistema BLE. Ahora no solo puedes hacer que tu ESP32 sea visible, sino que puede **comunicar información estructurada** a cualquier dispositivo compatible. Este patrón de servicios y características es la base de prácticamente todas las aplicaciones BLE profesionales.

El camino de aquí en adelante es emocionante: desde simples lectores de sensores hasta sistemas complejos de IoT, las posibilidades son infinitas. Experimenta, modifica, rompe y arregla el código. Cada iteración te acercará más a dominar esta poderosa tecnología.

**¡Feliz codificación y que tus señales BLE siempre sean fuertes! 📡✨**
