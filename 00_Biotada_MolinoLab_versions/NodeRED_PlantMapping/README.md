# Node-RED: Gestión de Dispositivos y Plantas

Este directorio contiene todos los archivos necesarios para implementar el sistema de gestión de asociaciones temporales entre dispositivos y plantas en Node-RED.

## Estructura de archivos

```
NodeRED_PlantMapping/
├── data/
│   └── device-plant-mapping.json    # Archivo de datos (se crea automáticamente)
├── functions/
│   ├── 01-read-plant-mapping.js     # Leer y buscar asociación activa
│   ├── 02-enrich-influxdb-tags.js   # Añadir tags a mensaje para InfluxDB
│   ├── 03-create-association.js     # Crear nueva asociación
│   ├── 04-get-active-association.js # Obtener asociación activa
│   ├── 05-get-all-associations.js   # Obtener todas las asociaciones
│   ├── 06-close-association.js      # Cerrar asociación activa
│   └── 07-get-all-plants-map.js     # Obtener plantas para mapa
└── README.md                         # Este archivo
```

## Instalación paso a paso

### Paso 1: Copiar archivo de datos

1. Copia el archivo `data/device-plant-mapping.json` a tu directorio de datos de Node-RED:
   - **Linux/Mac**: `~/.node-red/data/device-plant-mapping.json`
   - **Windows**: `C:\Users\{tu_usuario}\.node-red\data\device-plant-mapping.json`

   O simplemente crea el archivo con este contenido:
   ```json
   {
     "associations": []
   }
   ```

### Paso 2: Crear funciones en Node-RED

Para cada archivo en la carpeta `functions/`:

1. Abre Node-RED en tu navegador
2. Haz clic en el menú (☰) → **Import**
3. Selecciona **Functions** en lugar de Flows
4. Copia el contenido del archivo `.js` correspondiente
5. Pega en el editor y guarda

**O mejor aún**: Crea nodos Function manualmente y copia el código:

#### Función 1: Leer mapeo de planta
- Crea un nodo **Function**
- Nómbralo: `Leer mapeo planta`
- Copia el contenido de `01-read-plant-mapping.js`

#### Función 2: Enriquecer tags InfluxDB
- Crea un nodo **Function**
- Nómbralo: `Añadir tags planta`
- Copia el contenido de `02-enrich-influxdb-tags.js`

#### Función 3: Crear asociación
- Crea un nodo **Function**
- Nómbralo: `Crear asociación`
- Copia el contenido de `03-create-association.js`

#### Función 4: Obtener asociación activa
- Crea un nodo **Function**
- Nómbralo: `Obtener asociación activa`
- Copia el contenido de `04-get-active-association.js`

#### Función 5: Obtener todas las asociaciones
- Crea un nodo **Function**
- Nómbralo: `Obtener todas asociaciones`
- Copia el contenido de `05-get-all-associations.js`

#### Función 6: Cerrar asociación
- Crea un nodo **Function**
- Nómbralo: `Cerrar asociación`
- Copia el contenido de `06-close-association.js`

#### Función 7: Obtener plantas para mapa
- Crea un nodo **Function**
- Nómbralo: `Obtener plantas mapa`
- Copia el contenido de `07-get-all-plants-map.js`

### Paso 3: Integrar en flujo MQTT → InfluxDB

En tu flujo existente de MQTT a InfluxDB:

1. **Localiza tu nodo MQTT In** que recibe los mensajes de los dispositivos
2. **Añade después del nodo MQTT In**:
   - Nodo Function: `Leer mapeo planta` (función 1)
   - Nodo Function: `Añadir tags planta` (función 2)
3. **Conecta**: MQTT In → `Leer mapeo planta` → `Añadir tags planta` → InfluxDB Out

### Paso 4: Crear endpoints HTTP

Para cada endpoint, crea un flujo con estos nodos:

#### Endpoint 1: Crear asociación
```
HTTP In (POST /device-plant/associate)
  ↓
Function: Crear asociación (función 3)
  ↓
HTTP Response
```

#### Endpoint 2: Obtener asociación activa
```
HTTP In (GET /device-plant/active/:device_id)
  ↓
Function: Obtener asociación activa (función 4)
  ↓
HTTP Response
```

#### Endpoint 3: Obtener todas las asociaciones
```
HTTP In (GET /device-plant/associations/:device_id)
  ↓
Function: Obtener todas asociaciones (función 5)
  ↓
HTTP Response
```

#### Endpoint 4: Cerrar asociación
```
HTTP In (POST /device-plant/close/:device_id)
  ↓
Function: Cerrar asociación (función 6)
  ↓
HTTP Response
```

#### Endpoint 5: Obtener plantas para mapa
```
HTTP In (GET /api/plants/map)
  ↓
Function: Obtener plantas mapa (función 7)
  ↓
HTTP Response
```

## Uso

### Crear una asociación

**POST** `http://tu-nodered:1880/device-plant/associate`

```json
{
  "device_id": "biodata_12345",
  "plant_name": "Roble del Parque Central",
  "plant_species": "Quercus robur",
  "gps_latitude": 40.4168,
  "gps_longitude": -3.7038,
  "gps_altitude": 650.5,
  "start_time": "2024-01-15T08:00:00Z",
  "end_time": null,
  "additional_data": {
    "height_meters": 12.5,
    "notes": "Árbol centenario"
  }
}
```

### Obtener asociación activa

**GET** `http://tu-nodered:1880/device-plant/active/biodata_12345`

### Cerrar asociación activa

**POST** `http://tu-nodered:1880/device-plant/close/biodata_12345`

```json
{
  "end_time": "2024-01-22T08:00:00Z"
}
```

(O sin body para cerrar ahora)

### Obtener plantas para mapa

**GET** `http://tu-nodered:1880/api/plants/map`

## Notas importantes

- El archivo `device-plant-mapping.json` se crea automáticamente si no existe
- Las funciones usan caché en memoria (TTL 60 segundos) para optimizar lecturas
- Si un dispositivo no tiene asociación activa, se añade el tag `plant_name: "unknown"` a InfluxDB
- Las asociaciones históricas se mantienen en el archivo para consulta posterior

## Solución de problemas

### Error: "No se pudo extraer device_id"
- Verifica que el mensaje MQTT tenga el campo `sid` en el payload JSON
- O que el topic tenga el formato `{base}/{device_id}/midi` o `{base}/{device_id}`

### Error: "Error al leer archivo"
- Verifica que Node-RED tenga permisos de lectura/escritura en el directorio `data/`
- Verifica que la ruta del archivo sea correcta (usa `global.get('userDir')`)

### El tag "plant_name" siempre es "unknown"
- Verifica que exista una asociación activa para el dispositivo
- Verifica que los timestamps `start_time` y `end_time` sean correctos (ISO 8601)

