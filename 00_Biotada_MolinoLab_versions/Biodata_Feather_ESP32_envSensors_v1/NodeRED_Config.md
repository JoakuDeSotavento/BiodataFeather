# Configuración Node-RED para BiodataFeather Integrated

## Tópicos MQTT Utilizados

### Datos de Sensores
```
biodata/sensors/biodata1
```

### Comandos de Control
```
biodata/control/biodata1
```

## Configuración del Flujo Existente

Si ya tienes un flujo de Node-RED funcionando, solo necesitas actualizar los tópicos MQTT:

### 1. Actualizar MQTT In para sensores
- Tópico: `biodata/sensors/#` (para recibir de todos los sensores)
- O específico: `biodata/sensors/biodata1`

### 2. Actualizar MQTT Out para control
- Tópico: `biodata/control/biodata1`

## Formato de Datos

### Datos de Sensores (JSON)
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

### Comandos de Control (JSON)
```json
{
  "threshold": 5,        // Cambiar threshold de detección
  "scale": 2,           // Cambiar escala (0-4)
  "midi_channel": 3     // Cambiar canal MIDI
}
```

## Configuración para Múltiples Dispositivos

Para cada dispositivo, cambia el `SENSOR_ID` en `secrets.h`:
- Dispositivo 1: `#define SENSOR_ID "biodata1"`
- Dispositivo 2: `#define SENSOR_ID "biodata2"`
- Dispositivo 3: `#define SENSOR_ID "biodata3"`

Los tópicos serán:
- `biodata/sensors/biodata1`
- `biodata/sensors/biodata2`
- `biodata/sensors/biodata3`

## Integración con InfluxDB

El flujo existente debería funcionar sin cambios si:
1. Los tópicos MQTT coinciden
2. El formato JSON es compatible
3. La configuración de InfluxDB está correcta

## Verificación

Para verificar que todo funciona:

1. **En el ESP32**: Deberías ver en el serial:
   ```
   Datos enviados por MQTT:
   {"temperatura":23.45,"presion":101325,...}
   ```

2. **En Node-RED**: Los mensajes deberían llegar al nodo MQTT In

3. **En InfluxDB**: Los datos deberían aparecer en la base de datos

## Troubleshooting

### No llegan datos a Node-RED
- Verifica que el broker MQTT esté funcionando
- Comprueba que los tópicos coincidan
- Revisa la conexión WiFi del ESP32

### Error en InfluxDB
- Verifica la configuración de la base de datos
- Comprueba que el formato de datos sea correcto
- Revisa los logs de Node-RED
