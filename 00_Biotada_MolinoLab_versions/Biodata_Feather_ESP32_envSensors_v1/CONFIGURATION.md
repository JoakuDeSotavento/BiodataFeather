# Configuración de BiodataFeather Integrated v1.0

## Archivos de Configuración

### 1. `secrets.h` - Información Sensible
**SOLO** credenciales y URLs:
```cpp
#define WIFI_SSID "tu_wifi_ssid"
#define WIFI_PASSWORD "tu_wifi_password"
#define MQTT_BROKER "tu_mqtt_broker.com"
#define MQTT_PORT 1883
#define MQTT_USERNAME "tu_usuario_mqtt"   // opcional
#define MQTT_PASSWORD "tu_password_mqtt"  // opcional
```

### 2. `Biodata_Feather_ESP32_Integrated_v1.ino` - Configuración del Sistema

#### Identificación del Dispositivo
```cpp
#define SENSOR_ID "biodata1" // Cambia para cada dispositivo
```

#### Intervalos de Lectura
```cpp
#define SENSOR_READ_INTERVAL 5000    // 5 segundos
#define MQTT_SEND_INTERVAL 10000     // 10 segundos
```

#### Configuración MIDI
```cpp
#define DEFAULT_MIDI_CHANNEL 1
#define DEFAULT_THRESHOLD 3
#define DEFAULT_SCALE 3  // 0=Cromática, 1=Menor, 2=Mayor, 3=Pentatónica, 4=India
```

#### Configuración de LEDs
```cpp
#define LED_BRIGHTNESS_MAX 255
#define LED_BRIGHTNESS_DIM 50
#define LED_BRIGHTNESS_ACTIVITY 100
```

#### Calibración de Presión (Opcional)
```cpp
// Descomenta para calibrar presión a nivel del mar
// #define CALIBRATE_PRESSURE
```

## Configuración para Múltiples Dispositivos

### Dispositivo 1
```cpp
#define SENSOR_ID "biodata1"
```

### Dispositivo 2
```cpp
#define SENSOR_ID "biodata2"
```

### Dispositivo 3
```cpp
#define SENSOR_ID "biodata3"
```

## Tópicos MQTT Resultantes

Con la configuración anterior, los tópicos serán:
- `biodata/sensors/biodata1`
- `biodata/sensors/biodata2`
- `biodata/sensors/biodata3`
- `biodata/control/biodata1`
- `biodata/control/biodata2`
- `biodata/control/biodata3`

## Escalas Musicales Disponibles

```cpp
// 0 = Cromática (todas las notas)
// 1 = Menor (escala menor natural)
// 2 = Mayor (escala mayor)
// 3 = Pentatónica (5 notas)
// 4 = India (escala india)
```

## Configuración de Sensores

### BME688 (Temperatura, Humedad, Presión, Gas)
- **Dirección I2C**: 0x77 (por defecto)
- **Pines**: SDA=21, SCL=20
- **Voltaje**: 3.3V

### LTR329 (Luz Visible e Infrarroja)
- **Pines**: SDA=21, SCL=20
- **Voltaje**: 3.3V
- **Ganancia**: LTR3XX_GAIN_2
- **Tiempo de integración**: LTR3XX_INTEGTIME_100
- **Tasa de medición**: LTR3XX_MEASRATE_200

### Galvanómetro (Entrada Biológica)
- **Pin de señal**: 44
- **Tipo de interrupción**: RISING
- **Tamaño de muestra**: 32
- **Análisis**: 31 muestras

## Configuración de LEDs

### Mapeo de LEDs
```cpp
byte leds[5] = { 18, 17, 8, 36, 35 };
```

### Estados de LEDs
- **LED 0 (Pin 18)**: Rojo - Error/Desconexión WiFi
- **LED 1 (Pin 17)**: Azul - Estado WiFi
- **LED 2 (Pin 8)**: Verde - Estado MQTT
- **LED 3 (Pin 36)**: Blanco - Actividad de sensores
- **LED 4 (Pin 35)**: Amarillo - Actividad MIDI

## Configuración de Polifonía MIDI

```cpp
const byte polyphony = 5;  // Máximo 5 notas simultáneas
```

## Configuración de Notas MIDI

```cpp
int noteMin = 36;  // Nota más baja (C2)
int noteMax = 96;  // Nota más alta (C7)
```

## Configuración de Control MIDI

```cpp
byte controlNumber = 80;  // Número de control MIDI
```

## Optimización de Rendimiento

### Para Múltiples Dispositivos
1. **Aumentar intervalos** si hay muchos dispositivos:
   ```cpp
   #define SENSOR_READ_INTERVAL 10000   // 10 segundos
   #define MQTT_SEND_INTERVAL 30000     // 30 segundos
   ```

2. **Reducir debug** para mejor rendimiento:
   ```cpp
   byte debugSerial = 0;  // Desactivar debug
   ```

3. **Optimizar MQTT**:
   ```cpp
   mqtt.setKeepAlive(30);  // Keep-alive más largo
   ```

### Para Bajo Consumo
1. **Aumentar intervalos** de lectura
2. **Desactivar LEDs** innecesarios
3. **Reducir polifonía MIDI**
4. **Usar sleep mode** entre lecturas

## Troubleshooting

### Sensores No Detectados
- Verificar conexiones I2C
- Comprobar voltaje en VCC (3.3V)
- Verificar direcciones I2C

### MQTT No Conecta
- Verificar credenciales en `secrets.h`
- Comprobar que el broker esté funcionando
- Verificar puerto y firewall

### MIDI No Funciona
- Verificar librerías instaladas
- Comprobar conexiones USB
- Verificar configuración de canales

### LEDs No Funcionan
- Verificar conexiones de pines
- Comprobar voltaje de alimentación
- Verificar configuración de pines
