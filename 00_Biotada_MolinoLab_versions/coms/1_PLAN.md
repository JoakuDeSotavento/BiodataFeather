# Plan Técnico: Integración WiFiManager con Captive Portal

## Descripción
Implementar WiFiManager en la versión v7.5_OSC_lan del BiodataFeather para permitir configuración de WiFi, IP de destino OSC y puerto OSC mediante un captive portal web. El portal se activará al pulsar más de un segundo el botón del menú una vez encendido el dispositivo, y redirigirá automáticamente a la IP del ESP32 en lugar de MSN.

## Archivos a Modificar

### Archivo Principal
- `Biodata_Feather_ESP32_7_5_OSC_lan/Biodata_Feather_ESP32_7_5_OSC_lan.ino`

### Funciones a Modificar/Crear

#### Funciones Existentes a Modificar:
1. **`setup()`** - Añadir inicialización de WiFiManager
2. **`checkButton()`** - Modificar lógica del botón del menú para activar captive portal (pulsación >1 segundo)
3. **`setupWifi()`** - Reemplazar con lógica de WiFiManager
4. **Variables globales** - Añadir variables para WiFiManager y parámetros personalizados

#### Nuevas Funciones a Crear:
1. **`initWiFiManager()`** - Configurar WiFiManager con parámetros personalizados (SSID, contraseña, IP OSC, puerto OSC)
2. **`startCaptivePortal()`** - Activar portal cautivo manualmente
3. **`saveOSCConfig()`** - Guardar configuración OSC en EEPROM
4. **`checkMenuButtonLongPress()`** - Detectar pulsación larga del botón del menú
5. **`loadOSCConfig()`** - Cargar configuración OSC desde EEPROM

## Algoritmo de Implementación

### Fase 1: Configuración de WiFiManager
1. **Incluir librería WiFiManager**
2. **Crear instancia global de WiFiManager**
3. **Definir parámetros personalizados**:
   - Campo "osc_ip" para IP de destino OSC
   - Campo "osc_port" para puerto OSC (por defecto 8000)
4. **Configurar callbacks** para guardar configuración

### Fase 2: Modificación del Sistema de Menús
1. **Detectar pulsación larga del botón del menú** (>1 segundo) una vez encendido
2. **Activar modo AP** y captive portal
3. **Mostrar indicador LED** durante configuración
4. **Redirigir navegador** a IP del ESP32 (no MSN)

### Fase 3: Persistencia de Configuración
1. **Guardar IP OSC y puerto OSC** en EEPROM
2. **Cargar configuración** al inicio
3. **Actualizar variables globales** `oscIP` y `oscPort`
4. **Mantener compatibilidad** con configuración existente

### Fase 4: Integración con Sistema Existente
1. **Preservar funcionalidad MIDI** completa
2. **Mantener sistema de LEDs** para indicar estado
3. **Conservar menú de configuración** existente
4. **Añadir opción "Configurar WiFi"** en menú

## Detalles Técnicos Específicos

### Variables a Añadir:
```cpp
#include <WiFiManager.h>
WiFiManager wm;
WiFiManagerParameter custom_osc_ip("osc_ip", "OSC IP", "192.168.1.170", 16);
WiFiManagerParameter custom_osc_port("osc_port", "OSC Port", "8000", 6);
bool captivePortalActive = false;
```

### Modificaciones en setup():
- Inicializar WiFiManager después de EEPROM
- Configurar parámetros personalizados
- Intentar conexión automática
- Si falla, activar portal cautivo

### Modificaciones en checkButton():
- Detectar pulsación larga del botón del menú (>1 segundo) una vez encendido
- En lugar de toggle WiFi, activar captive portal
- Mostrar LED verde parpadeante durante configuración

### Modificaciones en setupWifi():
- Reemplazar lógica de conexión manual
- Usar WiFiManager.autoConnect()
- Configurar callbacks para guardar parámetros

### Algoritmo de Captive Portal:
1. **Usuario pulsa botón del menú** más de 1 segundo una vez encendido
2. **ESP32 activa modo AP** con nombre "BiodataFeather-Config"
3. **Navegador se redirige** automáticamente a 192.168.4.1
4. **Usuario configura** SSID, contraseña, IP OSC y puerto OSC
5. **WiFiManager guarda** configuración en EEPROM
6. **ESP32 se reinicia** y conecta con nueva configuración

### Persistencia en EEPROM:
- **Dirección 4**: IP OSC (4 bytes)
- **Dirección 8**: Puerto OSC (2 bytes, por defecto 8000)
- **Mantener direcciones existentes**: 0-3 para configuración MIDI

### Indicadores LED:
- **LED Verde (pin 8)**: Parpadea durante configuración
- **LED Rojo (pin 18)**: Error de conexión
- **LED Azul (pin 17)**: WiFi conectado

## Consideraciones de Compatibilidad

### Mantener Funcionalidad Existente:
- Sistema de menús con botón y potenciómetro
- Configuración MIDI (canales, escalas)
- Salidas MIDI (USB, BLE, Serial)
- Sistema de LEDs y feedback visual
- Análisis de datos biológicos

### No Modificar:
- Funciones de análisis de muestras
- Sistema de escalas musicales
- Configuración de pines y hardware
- Lógica de MIDI y OSC existente

### Añadir Funcionalidad:
- Captive portal para configuración WiFi
- Campo adicional para IP de destino OSC
- Persistencia de configuración OSC
- Redirección automática del navegador
