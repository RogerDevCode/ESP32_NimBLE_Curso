# Proyecto 2 - BLE Advertising (Anuncio BLE)

Este proyecto configura un ESP32 como servidor BLE básico que se anuncia con el nombre "ESP32-NimBLE" para que otros dispositivos puedan descubrirlo.

## Descripción

El dispositivo ESP32 utiliza el stack NimBLE para implementar funcionalidad Bluetooth Low Energy (BLE). El proyecto demuestra cómo:
- Inicializar el stack NimBLE
- Configurar parámetros de advertising
- Anunciarse con un nombre específico
- Permitir que otros dispositivos BLE descubran el ESP32

## Requisitos

- **ESP-IDF**: v5.5.1 o superior
- **Hardware**: ESP32 (cualquier variante con Bluetooth)
- **Extensión de VS Code**: ESP-IDF (opcional pero recomendado)

## Estructura del Proyecto

```
2_ble_advertising/
├── .vscode/                    # Configuración de VS Code
│   ├── c_cpp_properties.json   # Configuración de IntelliSense
│   ├── settings.json           # Configuración del editor
│   ├── tasks.json              # Tareas de compilación y flash
│   └── launch.json             # Configuración de depuración
├── main/
│   ├── CMakeLists.txt          # Configuración del componente main
│   └── main.c                  # Código principal del proyecto
├── CMakeLists.txt              # Configuración principal de CMake
├── sdkconfig.defaults          # Configuración predeterminada de NimBLE
└── README.md                   # Este archivo
```

## Configuración

El archivo `sdkconfig.defaults` contiene la configuración predeterminada para NimBLE:
- Bluetooth habilitado
- Stack NimBLE habilitado (Bluedroid deshabilitado)
- Rol periférico y broadcaster habilitados
- Nombre del dispositivo: "ESP32-NimBLE"

## Compilación y Flash

### Usando idf.py (línea de comandos)

1. Configurar el entorno ESP-IDF:
   ```bash
   . $HOME/esp/v5.5.1/esp-idf/export.sh
   ```

2. Configurar el objetivo (solo la primera vez):
   ```bash
   idf.py set-target esp32
   ```

3. Compilar el proyecto:
   ```bash
   idf.py build
   ```

4. Flashear el dispositivo:
   ```bash
   idf.py -p /dev/ttyUSB0 flash
   ```

5. Monitorear la salida:
   ```bash
   idf.py -p /dev/ttyUSB0 monitor
   ```

### Usando VS Code

1. Abrir el proyecto en VS Code
2. Presionar `Ctrl+Shift+P` y ejecutar:
   - `ESP-IDF: Set Espressif device target` → Seleccionar ESP32
   - `ESP-IDF: Build your project` → Compilar
   - `ESP-IDF: Flash your project` → Flashear
   - `ESP-IDF: Monitor your device` → Ver logs

O usar las tareas predefinidas:
- `Ctrl+Shift+B` → Build
- Ver tareas disponibles en el menú Terminal → Run Task

## Prueba del Proyecto

1. Flashear el código al ESP32
2. Abrir la aplicación "nRF Connect" o similar en tu smartphone
3. Buscar dispositivos BLE
4. Deberías ver aparecer "ESP32-NimBLE" en la lista de dispositivos

## Logs Esperados

```
I (xxx) BLE_ADV: Inicializando NimBLE Port.
I (xxx) BLE_ADV: Tarea del Host BLE iniciada.
I (xxx) BLE_ADV: BLE Host sincronizado.
I (xxx) BLE_ADV: Advertising iniciado con el nombre: ESP32-NimBLE
I (xxx) BLE_ADV: app_main finalizado. La lógica BLE corre en su propia tarea.
```

## Componentes NimBLE Utilizados

- **nimble_port.h**: Inicialización del stack
- **ble_hs.h**: Host de NimBLE
- **ble_gap_adv_start()**: Inicia el advertising
- **ble_svc_gap_device_name_set()**: Establece el nombre del dispositivo

## Notas

- El dispositivo se anuncia continuamente hasta que se apague
- El modo de conexión es "undirected connectable" (BLE_GAP_CONN_MODE_UND)
- El modo de descubrimiento es "general" (BLE_GAP_DISC_MODE_GEN)
- El proyecto usa el componente NVS Flash para almacenar información de BLE

## Solución de Problemas

### Error al compilar
- Asegúrate de que ESP-IDF está correctamente instalado
- Verifica que el entorno esté configurado con `export.sh`
- Ejecuta `idf.py fullclean` y vuelve a compilar

### No aparece el dispositivo en el escáner BLE
- Verifica que el ESP32 se haya flasheado correctamente
- Revisa los logs con `idf.py monitor`
- Asegúrate de que Bluetooth esté habilitado en tu teléfono
- Intenta reiniciar el ESP32

### Puerto no encontrado
- Verifica que el cable USB esté conectado
- Comprueba el puerto correcto con `ls /dev/ttyUSB*` o `ls /dev/ttyACM*`
- Actualiza el puerto en `.vscode/settings.json` si es necesario

## Autor

Roger Gallegos (adaptado por IA)

## Fecha

08-12-2025
