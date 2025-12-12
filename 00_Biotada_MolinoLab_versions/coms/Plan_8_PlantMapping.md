# Plan 8: Gestión de Dispositivos y Plantas en Node-RED

## Descripción
Implementar en Node-RED un sistema de gestión que permita asociar dispositivos (identificados por `sensorID`) con plantas/árboles de forma temporal, almacenar sus coordenadas GPS y datos adicionales. **IMPORTANTE: Un dispositivo puede estar asociado a diferentes plantas en diferentes períodos de tiempo** (por ejemplo, una semana con una planta, al día siguiente con otra, o incluso durante 3 horas con otra planta). Antes de insertar datos en InfluxDB, Node-RED debe contrastar la correlación dispositivo/planta activa en el momento actual y añadir un tag con el nombre del ser vivo. Esta información debe ser consultable para visualización en página web y generación de mapas con la ubicación de las plantas.

## Archivos a crear/modificar

### Archivos nuevos (Node-RED):
- `flows/device-plant-mapping.json` - Flujo de Node-RED para gestión de dispositivos y plantas
- `data/device-plant-mapping.json` - Archivo JSON de almacenamiento persistente (ubicado en directorio de datos de Node-RED)

### Archivos de configuración:
- `config/device-plant-schema.json` - Esquema de datos para validación (opcional)

## Componentes necesarios en Node-RED

### 1. Almacenamiento de mapeo dispositivo/planta
**Estrategia**: Archivo JSON persistente almacenado en el directorio de datos de Node-RED.

**Ubicación del archivo**: 
- Ruta completa: `~/.node-red/data/device-plant-mapping.json` (Linux/Mac)
- Ruta completa: `C:\Users\{usuario}\.node-red\data\device-plant-mapping.json` (Windows)
- En Node-RED se accede mediante: `global.get('userDir')` o `process.env.NODE_RED_HOME`

**Estructura del archivo JSON** (array de asociaciones temporales, cada una con timestamps de inicio y fin):
```json
{
  "associations": [
    {
      "id": "assoc_001",
      "device_id": "biodata_12345",
      "plant_name": "Roble del Parque Central",
      "plant_species": "Quercus robur",
      "gps_latitude": 40.4168,
      "gps_longitude": -3.7038,
      "gps_altitude": 650.5,
      "additional_data": {
        "planted_date": "2020-03-15",
        "height_meters": 12.5,
        "diameter_cm": 45,
        "notes": "Árbol centenario, requiere monitoreo especial"
      },
      "start_time": "2024-01-15T08:00:00Z",
      "end_time": "2024-01-22T08:00:00Z",
      "created_at": "2024-01-15T08:00:00Z",
      "updated_at": "2024-01-15T08:00:00Z"
    },
    {
      "id": "assoc_002",
      "device_id": "biodata_12345",
      "plant_name": "Pino Mediterráneo",
      "plant_species": "Pinus halepensis",
      "gps_latitude": 40.4175,
      "gps_longitude": -3.7045,
      "gps_altitude": 655.0,
      "additional_data": {
        "planted_date": "2018-05-20",
        "height_meters": 8.2,
        "notes": "Requiere riego frecuente"
      },
      "start_time": "2024-01-22T08:00:00Z",
      "end_time": null,
      "created_at": "2024-01-22T08:00:00Z",
      "updated_at": "2024-01-22T08:00:00Z"
    },
    {
      "id": "assoc_003",
      "device_id": "biodata_12345",
      "plant_name": "Ciprés de Monterrey",
      "plant_species": "Cupressus macrocarpa",
      "gps_latitude": 40.4190,
      "gps_longitude": -3.7050,
      "gps_altitude": 660.0,
      "additional_data": {
        "notes": "Monitoreo temporal de 3 horas"
      },
      "start_time": "2024-01-23T10:00:00Z",
      "end_time": "2024-01-23T13:00:00Z",
      "created_at": "2024-01-23T10:00:00Z",
      "updated_at": "2024-01-23T10:00:00Z"
    },
    {
      "id": "assoc_004",
      "device_id": "biodata_67890",
      "plant_name": "Olivo Milenario",
      "plant_species": "Olea europaea",
      "gps_latitude": 40.4200,
      "gps_longitude": -3.7060,
      "gps_altitude": 670.0,
      "additional_data": {
        "age_years": 500,
        "notes": "Árbol histórico"
      },
      "start_time": "2024-01-20T00:00:00Z",
      "end_time": null,
      "created_at": "2024-01-20T00:00:00Z",
      "updated_at": "2024-01-20T00:00:00Z"
    }
  ]
}
```

**Campos importantes**:
- `id`: Identificador único de la asociación (generado automáticamente)
- `device_id`: ID del dispositivo sensor
- `start_time`: Timestamp ISO 8601 de inicio de la asociación (inclusivo)
- `end_time`: Timestamp ISO 8601 de fin de la asociación (exclusivo), `null` si la asociación está activa
- `plant_name`: Nombre de la planta/árbol
- `gps_latitude`, `gps_longitude`, `gps_altitude`: Coordenadas GPS de la planta
- `additional_data`: Objeto JSON con datos adicionales personalizados

**Reglas de asociación temporal**:
- Una asociación está **activa** si: `start_time <= ahora` y (`end_time` es `null` o `end_time > ahora`)
- Puede haber múltiples asociaciones para el mismo dispositivo en diferentes períodos
- Las asociaciones pueden ser consecutivas o tener gaps entre ellas
- Si hay múltiples asociaciones activas simultáneamente para el mismo dispositivo, se usa la más reciente (mayor `start_time`)

**Ventajas del archivo JSON**:
- ✅ Fácil de modificar manualmente con cualquier editor de texto
- ✅ Sin dependencias adicionales (usa módulos nativos de Node.js: `fs`)
- ✅ Fácil de hacer backup (simplemente copiar el archivo)
- ✅ Legible por humanos y versionable con Git
- ✅ Persistente entre reinicios de Node-RED

**Consideraciones**:
- ⚠️ Implementar lectura/escritura atómica para evitar corrupción en escrituras concurrentes
- ⚠️ El archivo se crea automáticamente si no existe (con estructura `{"associations": []}`)
- ⚠️ El directorio `data/` debe existir (Node-RED lo crea automáticamente, pero verificar permisos)
- ⚠️ **Gestión de asociaciones activas**: Al crear una nueva asociación, puede ser necesario cerrar automáticamente la asociación activa anterior del mismo dispositivo (opcional, según requerimientos)
- ⚠️ **Validación de solapamientos**: Validar que no haya solapamientos no deseados si se requiere (o permitirlos si se quiere monitoreo simultáneo)

**Acceso al archivo desde Node-RED**:
```javascript
// Obtener directorio de usuario de Node-RED
const userDir = global.get('userDir') || process.env.NODE_RED_HOME || '.';
const filePath = require('path').join(userDir, 'data', 'device-plant-mapping.json');

// En Node-RED, global.get('userDir') retorna la ruta del directorio de usuario
// Por defecto: ~/.node-red/ (Linux/Mac) o C:\Users\{usuario}\.node-red\ (Windows)
```

**Inicialización del archivo**:
- Si el archivo no existe al leerlo, se crea automáticamente con contenido `{}`
- Si el directorio `data/` no existe, se crea con `fs.mkdirSync(dir, { recursive: true })`
- El archivo se formatea con indentación de 2 espacios para legibilidad humana

### 2. Interfaz de gestión (Dashboard Node-RED)
**Nodos necesarios**:
- `node-red-dashboard` (ui_base, ui_template, ui_form, ui_table)
- Formularios para:
  - Asociar dispositivo con planta (dropdown de dispositivos + input de nombre de planta)
  - Editar coordenadas GPS (latitud, longitud, altitud)
  - Añadir/editar datos adicionales (campos dinámicos o JSON)

**Flujo de interfaz**:
```
HTTP In (GET /device-plant/list) → Query almacenamiento → HTTP Response (JSON)
HTTP In (POST /device-plant/create) → Validar datos → Guardar → HTTP Response
HTTP In (PUT /device-plant/update/:device_id) → Actualizar → HTTP Response
HTTP In (GET /device-plant/get/:device_id) → Consultar → HTTP Response
```

### 3. Procesamiento de datos MQTT antes de InfluxDB
**Ubicación**: Entre el nodo MQTT In y el nodo InfluxDB Out

**Algoritmo**:
```
1. Recibir mensaje MQTT con sensorID (campo "sid" en JSON o del topic)
2. Extraer sensorID del mensaje:
   - Si viene en JSON: msg.payload.sid
   - Si viene en topic: extraer de topic pattern "{base}/{sensorID}/midi" o "{base}/{sensorID}"
3. Consultar almacenamiento de mapeo usando sensorID como clave
4. Si existe mapeo:
   a. Extraer plant_name del registro
   b. Añadir tag "plant_name" al punto de InfluxDB
   c. Opcionalmente añadir tags adicionales: "plant_species", "gps_lat", "gps_lon"
5. Si no existe mapeo:
   a. Añadir tag "plant_name" con valor "unknown" o dejar sin tag
   b. Opcionalmente loggear advertencia
6. Enviar punto a InfluxDB con tags añadidos
```

**Implementación en Node-RED**:
- Nodo `function` después de MQTT In que:
  - Extrae `sensorID` del mensaje
  - Lee el archivo JSON desde el sistema de archivos
  - Busca la asociación activa para el `sensorID` en el momento actual
  - Añade propiedades al `msg` para InfluxDB:
    ```javascript
    const fs = require('fs');
    const path = require('path');
    
    // Obtener ruta del archivo JSON
    const userDir = global.get('userDir') || process.env.NODE_RED_HOME || '.';
    const filePath = path.join(userDir, 'data', 'device-plant-mapping.json');
    
    // Leer asociaciones
    let data = { associations: [] };
    try {
        const fileData = fs.readFileSync(filePath, 'utf8');
        data = JSON.parse(fileData);
    } catch (err) {
        // Archivo no existe o está corrupto, usar estructura vacía
        data = { associations: [] };
    }
    
    // Extraer sensorID
    const sensorID = msg.payload.sid || msg.topic.split('/')[1];
    const now = new Date().toISOString();
    
    // Buscar asociación activa para este dispositivo
    // Una asociación está activa si: start_time <= ahora y (end_time es null o end_time > ahora)
    const activeAssociations = data.associations.filter(assoc => {
        return assoc.device_id === sensorID &&
               assoc.start_time <= now &&
               (assoc.end_time === null || assoc.end_time > now);
    });
    
    // Si hay múltiples asociaciones activas, usar la más reciente (mayor start_time)
    let plantMapping = null;
    if (activeAssociations.length > 0) {
        activeAssociations.sort((a, b) => new Date(b.start_time) - new Date(a.start_time));
        plantMapping = activeAssociations[0];
    }
    
    // Añadir tags para InfluxDB
    msg.tags = msg.tags || {};
    if (plantMapping) {
        msg.tags.plant_name = plantMapping.plant_name;
        if (plantMapping.plant_species) {
            msg.tags.plant_species = plantMapping.plant_species;
        }
        if (plantMapping.gps_latitude !== undefined) {
            msg.tags.gps_lat = plantMapping.gps_latitude.toString();
        }
        if (plantMapping.gps_longitude !== undefined) {
            msg.tags.gps_lon = plantMapping.gps_longitude.toString();
        }
        // Opcional: añadir ID de asociación para trazabilidad
        msg.tags.association_id = plantMapping.id;
    } else {
        msg.tags.plant_name = "unknown";
    }
    
    return msg;
    ```

### 4. API para consulta de datos (para página web y mapas)
**Endpoints necesarios**:
- `GET /api/plants` - Listar todas las plantas con sus dispositivos asociados
- `GET /api/plants/:device_id` - Obtener información de planta por device_id
- `GET /api/plants/map` - Obtener datos para mapa (coordenadas + nombres)
- `GET /api/devices` - Listar todos los dispositivos y su estado de mapeo

**Formato de respuesta para mapa** (solo asociaciones activas):
```json
{
  "plants": [
    {
      "device_id": "biodata_12345",
      "association_id": "assoc_002",
      "plant_name": "Pino Mediterráneo",
      "plant_species": "Pinus halepensis",
      "gps_latitude": 40.4175,
      "gps_longitude": -3.7045,
      "gps_altitude": 655.0,
      "start_time": "2024-01-22T08:00:00Z",
      "end_time": null,
      "last_data_timestamp": "2024-01-23T14:30:00Z"
    }
  ]
}
```

**Nota**: Solo se incluyen asociaciones activas en el momento de la consulta. Para obtener historial completo, usar endpoint `/api/plants/associations/:device_id`.

**Implementación**:
- Usar nodos `http in` y `http response` de Node-RED
- Función JavaScript que consulta almacenamiento y formatea respuesta
- Opcionalmente añadir caché para mejorar rendimiento

## Algoritmos paso a paso

### Algoritmo 1: Extracción de sensorID desde mensaje MQTT
```
Entrada: msg (mensaje MQTT de Node-RED)
Salida: sensorID (string)

1. Si msg.payload es objeto JSON y contiene campo "sid":
   sensorID = msg.payload.sid
2. Si no, extraer de msg.topic:
   - Patrón topic: "{MQTT_BASE_TOPIC}/{sensorID}/midi" o "{MQTT_BASE_TOPIC}/{sensorID}"
   - Dividir topic por "/"
   - sensorID = partes[1] (índice después de base topic)
3. Si no se encuentra, usar msg.topic completo como fallback
4. Retornar sensorID
```

### Algoritmo 2: Lectura del archivo JSON y búsqueda de asociación activa
```
Entrada: sensorID (string), timestampActual (ISO 8601 string, opcional)
Salida: plantMapping (objeto asociación activa o null)

1. Construir ruta del archivo: {userDir}/data/device-plant-mapping.json
2. Intentar leer archivo con fs.readFileSync()
3. Si archivo existe:
   a. Parsear JSON (estructura: {associations: [...]})
   b. Obtener timestamp actual (timestampActual o new Date().toISOString())
   c. Filtrar asociaciones donde:
      - device_id === sensorID
      - start_time <= timestampActual
      - end_time === null O end_time > timestampActual
   d. Si hay múltiples asociaciones activas:
      - Ordenar por start_time descendente (más reciente primero)
      - Seleccionar la primera (más reciente)
   e. Retornar objeto plantMapping o null si no hay asociación activa
4. Si archivo no existe o hay error:
   a. Retornar null
   b. Loggear advertencia si es necesario
```

### Algoritmo 3: Consulta y enriquecimiento de datos para InfluxDB
```
Entrada: msg (mensaje MQTT), filePath (ruta al archivo JSON)
Salida: msg enriquecido con tags de planta

1. sensorID = extraerSensorID(msg)
2. timestampActual = new Date().toISOString()
3. data = leerArchivoJSON(filePath) // Retorna {associations: [...]}
4. activeAssociations = filtrar asociaciones activas para sensorID en timestampActual
5. Si hay asociaciones activas:
   a. Ordenar por start_time descendente
   b. plantMapping = primera asociación (más reciente)
   c. msg.tags.plant_name = plantMapping.plant_name
   d. msg.tags.plant_species = plantMapping.plant_species (si existe)
   e. msg.tags.gps_lat = plantMapping.gps_latitude.toString() (si existe)
   f. msg.tags.gps_lon = plantMapping.gps_longitude.toString() (si existe)
   g. msg.tags.association_id = plantMapping.id (opcional, para trazabilidad)
6. Si no hay asociación activa:
   msg.tags.plant_name = "unknown"
7. Retornar msg
```

### Algoritmo 4: Crear nueva asociación temporal
```
Entrada: deviceId, plantData (objeto con datos de planta), startTime (ISO 8601), endTime (ISO 8601 o null)
Salida: éxito/error, id de asociación creada

1. Construir ruta del archivo: {userDir}/data/device-plant-mapping.json
2. Leer archivo actual (o crear {associations: []} si no existe)
3. Parsear JSON
4. Generar ID único para nueva asociación (ej: "assoc_" + timestamp + "_" + random)
5. Validar datos:
   a. deviceId no vacío
   b. plant_name no vacío
   c. startTime válido (ISO 8601)
   d. endTime válido o null
   e. Si endTime no es null: endTime > startTime
6. Crear objeto asociación:
   {
     id: nuevoId,
     device_id: deviceId,
     plant_name: plantData.plant_name,
     plant_species: plantData.plant_species,
     gps_latitude: plantData.gps_latitude,
     gps_longitude: plantData.gps_longitude,
     gps_altitude: plantData.gps_altitude,
     additional_data: plantData.additional_data || {},
     start_time: startTime,
     end_time: endTime,
     created_at: new Date().toISOString(),
     updated_at: new Date().toISOString()
   }
7. (Opcional) Cerrar asociación activa anterior del mismo dispositivo:
   - Buscar asociaciones activas para deviceId
   - Si existe y endTime no es null y startTime > asociación anterior.end_time:
     - Actualizar end_time de asociación anterior a startTime
8. Añadir nueva asociación al array: data.associations.push(nuevaAsociacion)
9. Escribir archivo con fs.writeFileSync() con formato legible
10. Retornar éxito con id de asociación
```

### Algoritmo 5: Actualizar asociación existente
```
Entrada: associationId, plantDataActualizado, startTimeActualizado, endTimeActualizado
Salida: éxito/error

1. Leer archivo JSON
2. Buscar asociación con id === associationId
3. Si no existe: retornar error 404
4. Actualizar campos:
   - plant_name, plant_species, gps_*, additional_data si se proporcionan
   - start_time, end_time si se proporcionan
   - updated_at = new Date().toISOString()
5. Validar que end_time > start_time si ambos están presentes
6. Escribir archivo
7. Retornar éxito
```

### Algoritmo 6: Cerrar asociación activa (finalizar monitoreo)
```
Entrada: deviceId, endTime (ISO 8601, opcional, por defecto ahora)
Salida: éxito/error

1. Leer archivo JSON
2. Buscar asociaciones activas para deviceId (end_time === null)
3. Si no hay asociaciones activas: retornar error
4. Si hay múltiples asociaciones activas:
   - Seleccionar la más reciente (mayor start_time)
5. Actualizar end_time de la asociación seleccionada a endTime (o ahora si no se proporciona)
6. Actualizar updated_at
7. Escribir archivo
8. Retornar éxito
```

### Algoritmo 7: Validación de coordenadas GPS
```
Entrada: latitude, longitude
Salida: boolean (válido o no)

1. Si latitude es null/undefined: retornar false
2. Si longitude es null/undefined: retornar false
3. Si latitude < -90 o latitude > 90: retornar false
4. Si longitude < -180 o longitude > 180: retornar false
5. Retornar true
```

## Flujo de Node-RED: Procesamiento de datos

### Flujo principal (procesamiento MQTT → InfluxDB):
```
MQTT In (topic: "{base}/+/midi" o "{base}/+")
  ↓
Function: Extraer sensorID
  ↓
Function: Consultar mapeo dispositivo/planta
  ↓
Function: Añadir tags de planta al mensaje
  ↓
InfluxDB Out (con tags añadidos)
```

### Flujo secundario (gestión CRUD):
```
HTTP In (POST /device-plant/associate)
  ↓
Function: Validar datos (device_id, plant_name, GPS, start_time, end_time opcional)
  ↓
Function: Leer archivo JSON, crear nueva asociación, (opcional) cerrar asociación activa anterior, escribir archivo
  ↓
HTTP Response (éxito/error con id de asociación)

HTTP In (GET /device-plant/associations)
  ↓
Function: Leer archivo JSON, filtrar por device_id (opcional), formatear como array
  ↓
HTTP Response (JSON con lista de asociaciones)

HTTP In (GET /device-plant/active/:device_id)
  ↓
Function: Leer archivo JSON, buscar asociación activa para device_id en momento actual
  ↓
HTTP Response (JSON con asociación activa o 404 si no hay)

HTTP In (GET /device-plant/associations/:device_id)
  ↓
Function: Leer archivo JSON, filtrar asociaciones por device_id (todas, no solo activas)
  ↓
HTTP Response (JSON con lista de asociaciones del dispositivo)

HTTP In (PUT /device-plant/association/:association_id)
  ↓
Function: Validar datos, leer archivo JSON, actualizar asociación existente, escribir archivo
  ↓
HTTP Response (éxito/error)

HTTP In (POST /device-plant/close/:device_id)
  ↓
Function: Leer archivo JSON, cerrar asociación activa del dispositivo (establecer end_time), escribir archivo
  ↓
HTTP Response (éxito/error)

HTTP In (DELETE /device-plant/association/:association_id)
  ↓
Function: Leer archivo JSON, eliminar asociación por id, escribir archivo
  ↓
HTTP Response (éxito/error)
```

## Consideraciones técnicas

### Persistencia de datos
- **Archivo JSON**: Almacenado en `{userDir}/data/device-plant-mapping.json`
- **Estructura**: Array de asociaciones temporales `{associations: [...]}`
- **Escritura atómica**: Usar `fs.writeFileSync()` con archivo temporal y renombrado para evitar corrupción
- **Backup**: El archivo puede copiarse manualmente o implementar backup automático periódico
- **Inicialización**: Si el archivo no existe, se crea automáticamente con estructura `{"associations": []}`
- **Historial**: Todas las asociaciones se mantienen (incluso las finalizadas) para consulta histórica

### Rendimiento
- **Caché en memoria**: Cachear el contenido del archivo JSON en context global de Node-RED con TTL (ej: 60 segundos)
- **Lectura optimizada**: Leer archivo completo una vez y buscar en memoria en lugar de leer en cada mensaje MQTT
- **Filtrado eficiente**: Filtrar asociaciones activas en memoria después de leer el archivo
- **Invalidación de caché**: Invalidar caché cuando se modifica el archivo vía endpoints HTTP
- **Estrategia de caché**:
  ```
  Si caché existe y no ha expirado:
    usar caché
  Si no:
    leer archivo JSON
    guardar en caché con timestamp
    usar caché
  ```
- **Consideración**: Con muchas asociaciones históricas, el archivo puede crecer. Considerar limpieza periódica de asociaciones muy antiguas (opcional)

### Validación de datos
- Validar formato de sensorID/device_id (no vacío, caracteres permitidos: alfanuméricos, guiones bajos)
- Validar coordenadas GPS (rango válido: lat -90 a 90, lon -180 a 180)
- Validar que plant_name no esté vacío y tenga longitud razonable
- Validar que additional_data sea un objeto válido (si se proporciona)
- **Validar timestamps**: start_time y end_time deben ser ISO 8601 válidos
- **Validar rango temporal**: Si end_time no es null, debe ser mayor que start_time
- **Validar solapamientos**: Opcionalmente validar que no haya solapamientos no deseados (o permitirlos según requerimientos)

### Manejo de errores
- **Lectura de archivo**: Si falla la lectura del archivo JSON, usar estructura vacía `{associations: []}` y continuar con tag "unknown"
- **Escritura de archivo**: Si falla la escritura, retornar error HTTP pero no bloquear procesamiento MQTT
- **Archivo corrupto**: Si el JSON está corrupto, loggear error y usar estructura vacía (considerar backup automático)
- **Permisos de archivo**: Verificar permisos de escritura en el directorio `data/` de Node-RED
- **Asociación no encontrada**: Si se busca una asociación por ID y no existe, retornar 404
- **Sin asociación activa**: Si un dispositivo no tiene asociación activa, usar tag "unknown" sin bloquear el flujo
- **Múltiples asociaciones activas**: Si hay múltiples asociaciones activas (caso edge), usar la más reciente y loggear advertencia

### Compatibilidad con flujos existentes
- No modificar estructura de mensajes MQTT existentes
- Añadir tags de forma no destructiva (no sobrescribir tags existentes)
- Mantener compatibilidad con dispositivos sin mapeo (usar "unknown")

## Integración con página web y mapas

### Datos necesarios para visualización web
- Lista de plantas con coordenadas GPS
- Última lectura de datos por dispositivo (timestamp)
- Estado de conexión del dispositivo (última vez que envió datos)

### Formato para mapa (GeoJSON opcional)
```json
{
  "type": "FeatureCollection",
  "features": [
    {
      "type": "Feature",
      "geometry": {
        "type": "Point",
        "coordinates": [-3.7038, 40.4168]
      },
      "properties": {
        "device_id": "biodata_12345",
        "plant_name": "Roble del Parque Central",
        "plant_species": "Quercus robur",
        "last_update": "2024-01-15T14:30:00Z"
      }
    }
  ]
}
```

### Endpoint para datos de mapa
- `GET /api/plants/map` - Retornar GeoJSON o JSON simple con coordenadas
- Incluir filtros opcionales: por especie, por rango de fechas de última actualización

## Dependencias de Node-RED

### Paquetes necesarios:
- `node-red-dashboard` - Para interfaz de gestión (opcional, solo si se usa Dashboard)
- `node-red-contrib-influxdb` - Ya debe estar instalado para InfluxDB
- `node-red-contrib-mqtt-broker` o `node-red-node-mqtt` - Ya debe estar instalado

### Módulos nativos de Node.js (no requieren instalación):
- `fs` - Para lectura/escritura de archivos JSON
- `path` - Para construcción de rutas de archivos

### Paquetes opcionales:
- `node-red-contrib-express` - Para API REST más robusta (opcional)

## Fases de implementación (si es necesario)

**Solo incluir fases si es una funcionalidad grande** - En este caso, se puede implementar en una sola fase, pero se puede dividir en:

### Fase 1: Almacenamiento y procesamiento básico
- Crear estructura de archivo JSON inicial (`device-plant-mapping.json` con `{associations: []}`)
- Implementar función de lectura de archivo JSON con manejo de errores
- Implementar función de extracción de sensorID desde mensajes MQTT
- Implementar función de búsqueda de asociación activa (filtrar por device_id y timestamp actual)
- Implementar función de consulta de asociación activa y enriquecimiento de tags
- Integrar función en flujo MQTT → InfluxDB existente (entre MQTT In e InfluxDB Out)
- Implementar caché en memoria para optimizar lecturas

### Fase 2: Interfaz de gestión
- Implementar función de creación de nueva asociación temporal con generación de ID único
- Implementar función de escritura en archivo JSON con validación de timestamps
- Implementar función de cierre de asociación activa (establecer end_time)
- Crear endpoints HTTP para gestión de asociaciones temporales:
  - POST /device-plant/associate (crear nueva asociación)
  - GET /device-plant/active/:device_id (obtener asociación activa)
  - GET /device-plant/associations/:device_id (obtener todas las asociaciones de un dispositivo)
  - POST /device-plant/close/:device_id (cerrar asociación activa)
  - PUT /device-plant/association/:association_id (actualizar asociación)
  - DELETE /device-plant/association/:association_id (eliminar asociación)
- Crear formularios en Node-RED Dashboard (opcional) con campos de fecha/hora para start_time y end_time
- Validación de datos de entrada (GPS, plant_name, timestamps, etc.)
- Manejo de errores en escritura de archivo

### Fase 3: API para consulta externa
- Implementar endpoints para página web (`/api/plants`, `/api/plants/map`)
- Filtrar asociaciones activas para mostrar en mapa (solo plantas actualmente monitoreadas)
- Opcionalmente permitir consulta histórica (todas las asociaciones de un dispositivo)
- Formato de datos para mapas (GeoJSON opcional) con información de asociación activa
- Optimización de caché y invalidación cuando se modifica archivo
- Documentación de endpoints para integración con página web
- Considerar endpoint para obtener historial de asociaciones de un dispositivo (útil para análisis)

