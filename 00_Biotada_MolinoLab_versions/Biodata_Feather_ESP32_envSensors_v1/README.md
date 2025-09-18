# BiodataFeather Integrated v1.0

## Descripción
Versión integrada del BiodataFeather que combina:
- **Sonificación MIDI** de datos biológicos (galvanómetro)
- **Sensores medioambientales** (BME688 + LTR329)
- **Comunicación MQTT** para envío de datos en tiempo real
- **Interfaz LED** para monitoreo de estado

## Características

### 🎵 Sonificación MIDI
- Análisis en tiempo real de impulsos eléctricos de plantas
- 5 escalas musicales: Cromática, Menor, Mayor, Pentatónica, India
- Polifonía de hasta 5 notas simultáneas
- Salidas MIDI: USB, BLE, WiFi (RTP/AppleMIDI), Serial
- Control de velocidad y duración de notas

### 🌱 Sensores Medioambientales
- **BME688**: Temperatura, humedad, presión atmosférica, gas
- **LTR329**: Luz visible e infrarroja
- Lectura cada 5 segundos
- Calibración automática de presión

### 📡 Comunicación MQTT
- Envío de datos cada 10 segundos
- Formato JSON con timestamp
- Reconexión automática
- Suscripción a comandos de control remoto
- Tópicos únicos por dispositivo (basado en MAC)

### 💡 Interfaz LED
- **LED 0 (Rojo)**: Estado de error/conexión
- **LED 1 (Azul)**: Estado WiFi
- **LED 2 (Verde)**: Estado MQTT
- **LED 3 (Blanco)**: Actividad de sensores
- **LED 4 (Amarillo)**: Actividad MIDI

## Hardware Requerido

### ESP32-S3 Feather
- Microcontrolador principal
- WiFi y BLE integrados
- USB para MIDI

### Sensores I2C
- **BME688**: Sensor ambiental (temperatura, humedad, presión, gas)
- **LTR329**: Sensor de luz
- Conexión por STEMMA QT o I2C directo

### Entrada Biológica
- **Galvanómetro** conectado al pin 44
- Amplificador de señal (opcional)

### LEDs
- 5 LEDs RGB conectados a los pines: 18, 17, 8, 36, 35

## Instalación

### 1. Librerías Requeridas
```cpp
// Sensores
#include "Adafruit_LTR329_LTR303.h"
#include "DFRobot_BME68x.h"

// Comunicación
#include <PubSubClient.h>
#include <ArduinoJson.h>

// MIDI
#include <Adafruit_TinyUSB.h>

// Sistema
#include <WiFi.h>
#include <Wire.h>
#include <EEPROM.h>
```

### 2. Configuración
1. Edita `secrets.h` con tus credenciales WiFi y MQTT (solo información sensible)
2. Para múltiples dispositivos, cambia `SENSOR_ID` en el archivo principal
3. Configura el broker MQTT (Node-RED, HiveMQ, etc.)

### 3. Conexiones
```
BME688:
- VCC → 3.3V
- GND → GND
- SDA → SDA (21)
- SCL → SCL (20)

LTR329:
- VCC → 3.3V
- GND → GND
- SDA → SDA (21)
- SCL → SCL (20)

Galvanómetro:
- Señal → Pin 44
- GND → GND

LEDs:
- LED 0 → Pin 18
- LED 1 → Pin 17
- LED 2 → Pin 8
- LED 3 → Pin 36
- LED 4 → Pin 35
```

## Configuración MQTT

### Estructura de Tópicos
```
biodata/sensors/{SENSOR_ID}     # Datos de sensores
biodata/control/{SENSOR_ID}     # Comandos de control
```

### Formato de Datos
```json
{
  "temperatura": 23.45,
  "presion": 101325,
  "humedad": 65.32,
  "gas": 123456,
  "altitud": 525.0,
  "altitudCalibrada": 525.0,
  "visible_ir": 1234,
  "infrarrojo": 567,
  "sensor": "biodata_feather",
  "timestamp": 1234567890
}
```

### Comandos de Control
```json
{
  "threshold": 5,        # Cambiar threshold de detección
  "scale": 2,           # Cambiar escala (0-4)
  "midi_channel": 3     # Cambiar canal MIDI
}
```

## Configuración Node-RED

### Flujo Básico
1. **MQTT In**: Suscribirse a `biodata/sensors/#`
2. **JSON**: Parsear mensajes JSON
3. **InfluxDB**: Almacenar datos en base de datos
4. **Dashboard**: Visualización en tiempo real

### Ejemplo de Flujo
```json
[
  {
    "id": "mqtt_in",
    "type": "mqtt in",
    "topic": "biodata/sensors/#",
    "qos": 1
  },
  {
    "id": "json_parse",
    "type": "json",
    "property": "payload"
  },
  {
    "id": "influxdb",
    "type": "influxdb out",
    "database": "biodata",
    "measurement": "sensors"
  }
]
```

## Monitoreo y Debug

### Serial Output
```
=== Datos de Sensores ===
Temperatura: 23.45°C
Presión: 101325 Pa
Humedad: 65.32%
Gas: 123456 Ohm
Altitud: 525.00 m
Luz Visible: 1234
Luz IR: 567
=========================

BIO: 45,12.34,67,89,90
MIDI: 90 60 80
```

### Estados LED
- **Secuencia de inicio**: Todos los LEDs se encienden secuencialmente
- **Estado normal**: LEDs indican estado del sistema
- **Actividad MIDI**: LED amarillo parpadea con cada nota
- **Envío MQTT**: LED verde parpadea brevemente

## Optimización para Múltiples Dispositivos

### Escalabilidad
- Cada dispositivo tiene ID único (MAC address)
- Tópicos MQTT separados por dispositivo
- Configuración independiente por dispositivo

### Recomendaciones
1. **Broker MQTT**: Usar broker dedicado (Mosquitto, HiveMQ)
2. **Base de datos**: InfluxDB para series temporales
3. **Visualización**: Grafana o Node-RED Dashboard
4. **Red**: WiFi 5GHz para mejor rendimiento

## Troubleshooting

### Problemas Comunes
1. **Sensores no detectados**: Verificar conexiones I2C
2. **MQTT no conecta**: Verificar credenciales y broker
3. **MIDI no funciona**: Verificar librerías USB
4. **LEDs no funcionan**: Verificar configuración PWM

### Debug
- Activar `debugSerial = 1` para ver mensajes detallados
- Usar `showSystemStatus()` para diagnóstico completo
- Verificar conexiones con multímetro

## Próximas Mejoras

### Funcionalidades Planificadas
- [ ] Calibración automática de sensores
- [ ] Modo de bajo consumo
- [ ] Almacenamiento local (SD card)
- [ ] Interfaz web de configuración
- [ ] Machine learning para detección de patrones
- [ ] Integración con APIs meteorológicas

### Optimizaciones
- [ ] Reducir consumo de energía
- [ ] Mejorar precisión de sensores
- [ ] Optimizar protocolo MQTT
- [ ] Añadir compresión de datos

## Licencia
Este proyecto está bajo la licencia del proyecto original BiodataFeather.

## Contacto
Para soporte técnico o colaboraciones, contacta a través de electricityforprogress.com
