# Comparativa de Versiones BiodataFeather

## Resumen Ejecutivo

Este documento presenta un análisis comparativo de las diferentes versiones del proyecto BiodataFeather desarrolladas por MolinoLab. El proyecto evoluciona desde una implementación básica de sonificación MIDI hasta una plataforma integrada de sensores ambientales y comunicación IoT.

## Versiones Analizadas

1. **Biodata_Feather_ESP32_07** - Versión base con MIDI
2. **Biodata_Feather_ESP32_07_wifi** - Versión con WiFi y OSC
3. **Biodata_Feather_ESP32_08_OSC** - Versión con Firebase y OSC
4. **Biodata_Feather_ESP32_7_5_OSC_lan** - Versión LAN con OSC
5. **Biodata_Feather_ESP32_i2c_08** - Versión con soporte I2C
6. **Biodata_Feather_ESP32_envSensors_v1** - Versión integrada completa
7. **PublishInfluxDB** - Versión especializada para InfluxDB

---

## Tabla Comparativa de Características

| Característica | v07 | v07_wifi | v08_OSC | v7.5_OSC_lan | i2c_08 | envSensors_v1 | InfluxDB |
|----------------|-----|----------|---------|--------------|--------|---------------|----------|
| **Sonificación MIDI** | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ❌ |
| **WiFi** | ❌ | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| **Bluetooth MIDI** | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ❌ |
| **USB MIDI** | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ | ❌ |
| **OSC** | ❌ | ✅ | ✅ | ✅ | ❌ | ❌ | ❌ |
| **Firebase** | ❌ | ❌ | ✅ | ❌ | ❌ | ❌ | ❌ |
| **MQTT** | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ | ✅ |
| **InfluxDB** | ❌ | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ |
| **Sensores Ambientales** | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ | ✅ |
| **I2C Support** | ❌ | ❌ | ❌ | ❌ | ✅ | ✅ | ✅ |
| **Node-RED Integration** | ❌ | ❌ | ❌ | ❌ | ❌ | ✅ | ✅ |
| **Documentación** | Básica | Básica | Básica | Básica | Básica | Completa | Básica |

---

## Análisis Detallado por Versión

### 1. Biodata_Feather_ESP32_07
**Versión Base - Sonificación MIDI Pura**

#### Características Principales:
- **Sonificación MIDI**: Análisis de impulsos eléctricos de plantas
- **Escalas Musicales**: 5 escalas (Cromática, Menor, Mayor, Pentatónica, India)
- **Polifonía**: Hasta 5 notas simultáneas
- **Salidas MIDI**: USB, BLE, Serial
- **Interfaz**: Botón + potenciómetro + 5 LEDs
- **Configuración**: EEPROM para persistencia

#### Limitaciones:
- Sin conectividad de red
- Sin sensores ambientales
- Configuración manual de WiFi

#### Casos de Uso:
- Prototipos iniciales
- Demostraciones offline
- Desarrollo de algoritmos de sonificación

---

### 2. Biodata_Feather_ESP32_07_wifi
**Versión con Conectividad WiFi**

#### Nuevas Características:
- **WiFi**: Conexión a redes inalámbricas
- **OSC**: Protocolo Open Sound Control
- **AppleMIDI**: Soporte RTP MIDI
- **IP Estática**: Configuración de red fija

#### Diferencias con v07:
- Añadido `#include <WiFi.h>`
- Función `setupWifi()` implementada
- Configuración de credenciales WiFi
- Soporte para AppleMIDI sobre WiFi

#### Casos de Uso:
- Presentaciones en red
- Integración con software de audio
- Control remoto básico

---

### 3. Biodata_Feather_ESP32_08_OSC
**Versión con Firebase y OSC Avanzado**

#### Nuevas Características:
- **Firebase**: Base de datos en tiempo real
- **OSC Mejorado**: Mensajes más complejos
- **Secrets.h**: Separación de credenciales
- **Configuración Modular**: Mejor organización del código

#### Diferencias con v07_wifi:
- Integración con Firebase Realtime Database
- Función `sendOSCMessage()` mejorada
- Archivo `secrets.h` para credenciales
- Soporte para autenticación Firebase

#### Casos de Uso:
- Aplicaciones IoT básicas
- Almacenamiento de datos en la nube
- Monitoreo remoto

---

### 4. Biodata_Feather_ESP32_7_5_OSC_lan
**Versión LAN con OSC**

#### Características:
- **Configuración LAN**: IP específica para redes locales
- **OSC Simplificado**: Enfoque en comunicación local
- **Firebase**: Mantiene integración con Firebase

#### Diferencias con v08_OSC:
- IP configurada para red local (192.168.1.120)
- Configuración optimizada para LAN
- Menos complejidad en la configuración

#### Casos de Uso:
- Redes locales cerradas
- Laboratorios y estudios
- Instalaciones artísticas

---

### 5. Biodata_Feather_ESP32_i2c_08
**Versión con Soporte I2C**

#### Nuevas Características:
- **I2C**: Soporte para sensores I2C
- **Timing de Sensores**: Sistema de polling para sensores
- **Pin de Interrupción**: Cambio de pin 12 a pin 44
- **Debug Desactivado**: Optimización de rendimiento

#### Diferencias con v08_OSC:
- Añadido soporte I2C básico
- Variables para timing de sensores
- Pin de interrupción modificado
- `debugSerial = 0` por defecto

#### Casos de Uso:
- Prototipos con sensores I2C
- Desarrollo de sensores ambientales
- Preparación para versión integrada

---

### 6. Biodata_Feather_ESP32_envSensors_v1
**Versión Integrada Completa**

#### Características Principales:
- **Sensores Ambientales**: BME688 + LTR329
- **MQTT**: Comunicación IoT robusta
- **Node-RED**: Integración con flujos de datos
- **Documentación Completa**: README y CONFIGURATION detallados
- **Sistema Modular**: Código bien estructurado

#### Sensores Integrados:
- **BME688**: Temperatura, humedad, presión, gas
- **LTR329**: Luz visible e infrarroja
- **Galvanómetro**: Entrada biológica (pin 44)

#### Comunicación:
- **MQTT**: Tópicos estructurados por dispositivo
- **JSON**: Formato de datos estandarizado
- **Reconexión Automática**: Manejo robusto de conexiones

#### Interfaz:
- **5 LEDs**: Estado del sistema
- **Configuración Remota**: Control vía MQTT
- **Monitoreo**: Debug completo del sistema

#### Casos de Uso:
- Instalaciones IoT completas
- Monitoreo ambiental
- Arte interactivo avanzado
- Investigación científica

---

### 7. PublishInfluxDB
**Versión Especializada para InfluxDB**

#### Características:
- **InfluxDB v2**: Integración directa con base de datos de series temporales
- **HTTPS**: Comunicación segura
- **Optimización**: Envíos frecuentes (< 1 segundo)
- **Sensores**: BME688 + LTR329
- **MQTT**: Comunicación dual (InfluxDB + MQTT)

#### Diferencias con envSensors_v1:
- Enfoque específico en InfluxDB
- Comunicación HTTPS directa
- Optimización para alta frecuencia
- Menos funcionalidades MIDI

#### Casos de Uso:
- Monitoreo científico de alta frecuencia
- Análisis de datos en tiempo real
- Aplicaciones de investigación

---

## Evolución del Proyecto

### Fase 1: Sonificación Básica (v07)
- Implementación core de sonificación MIDI
- Interfaz básica de usuario
- Múltiples salidas MIDI

### Fase 2: Conectividad (v07_wifi, v08_OSC, v7.5_OSC_lan)
- Añadida conectividad WiFi
- Integración con servicios en la nube
- Protocolos de comunicación (OSC, Firebase)

### Fase 3: Sensores (i2c_08)
- Preparación para sensores ambientales
- Soporte I2C básico
- Optimización de rendimiento

### Fase 4: Integración Completa (envSensors_v1)
- Sensores ambientales completos
- Comunicación IoT robusta
- Documentación profesional
- Sistema modular y escalable

### Fase 5: Especialización (InfluxDB)
- Optimización para casos de uso específicos
- Integración con bases de datos especializadas
- Alta frecuencia de muestreo

---

## Recomendaciones de Uso

### Para Prototipos y Demostraciones:
- **v07**: Sonificación básica offline
- **v07_wifi**: Demostraciones con conectividad

### Para Aplicaciones IoT Básicas:
- **v08_OSC**: Con Firebase para almacenamiento
- **v7.5_OSC_lan**: Para redes locales

### Para Desarrollo de Sensores:
- **i2c_08**: Base para añadir sensores I2C

### Para Aplicaciones Completas:
- **envSensors_v1**: Solución completa con sensores y IoT
- **InfluxDB**: Para monitoreo científico de alta frecuencia

---

## Consideraciones Técnicas

### Compatibilidad de Hardware:
- Todas las versiones usan ESP32-S3 Feather
- Pines de LEDs consistentes: 18, 17, 8, 36, 35
- Pin de interrupción: 12 (v07-v08) → 44 (i2c_08+)

### Librerías Requeridas:
- **Comunes**: WiFi, BLE, TinyUSB, MIDI
- **OSC**: OSCMessage
- **Firebase**: Firebase, ArduinoJson
- **Sensores**: Adafruit_LTR329_LTR303, DFRobot_BME68x
- **IoT**: PubSubClient, ArduinoJson

### Configuración de Red:
- **v07**: Sin red
- **v07_wifi+**: Credenciales WiFi hardcodeadas
- **v08_OSC+**: Archivo secrets.h
- **envSensors_v1**: Configuración modular completa

---

## Conclusiones

El proyecto BiodataFeather ha evolucionado significativamente desde su versión inicial, pasando de una herramienta de sonificación básica a una plataforma IoT completa. La versión **envSensors_v1** representa el estado más avanzado del proyecto, combinando:

1. **Sonificación MIDI** robusta y configurable
2. **Sensores ambientales** de alta calidad
3. **Comunicación IoT** moderna y escalable
4. **Documentación** profesional y completa
5. **Arquitectura modular** para fácil mantenimiento

La versión **InfluxDB** demuestra la flexibilidad del proyecto para adaptarse a casos de uso específicos, mientras que las versiones intermedias muestran la evolución gradual de las funcionalidades.

Para nuevos desarrollos, se recomienda usar **envSensors_v1** como base, ya que proporciona la funcionalidad más completa y la mejor documentación.
