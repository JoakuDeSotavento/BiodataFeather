# Opciones de Almacenamiento para Mapeo Dispositivo/Planta

## Comparativa rápida

| Opción | Facilidad Modificación | Persistencia | Escalabilidad | Complejidad Setup |
|--------|----------------------|--------------|---------------|-------------------|
| **1. Archivo JSON** | ⭐⭐⭐⭐⭐ Muy fácil | ✅ Persistente | ⚠️ Limitada | ⭐ Muy simple |
| **2. Google Sheets** | ⭐⭐⭐⭐⭐ Muy fácil | ✅ Persistente | ✅ Buena | ⭐⭐ Simple |
| **3. Context Global** | ⭐⭐⭐⭐ Fácil | ⚠️ Volátil | ⚠️ Limitada | ⭐ Muy simple |
| **4. SQLite** | ⭐⭐⭐ Moderada | ✅ Persistente | ✅ Buena | ⭐⭐⭐ Media |
| **5. Airtable** | ⭐⭐⭐⭐⭐ Muy fácil | ✅ Persistente | ✅ Excelente | ⭐⭐ Simple |

---

## Opción 1: Archivo JSON (⭐ RECOMENDADA para empezar)

### Ventajas
- ✅ **Muy fácil de modificar**: Puedes editar el archivo directamente con cualquier editor de texto
- ✅ **Sin dependencias externas**: Solo necesitas Node-RED básico
- ✅ **Fácil de hacer backup**: Simplemente copiar el archivo
- ✅ **Legible por humanos**: Puedes ver y editar los datos directamente
- ✅ **Versionable con Git**: Puedes versionar cambios fácilmente

### Desventajas
- ⚠️ No ideal para muchas escrituras concurrentes (pero suficiente para gestión manual)
- ⚠️ Sin validación automática de datos
- ⚠️ Puede corromperse si hay escrituras simultáneas (mitigable con locking)

### Estructura del archivo
**Ubicación**: `~/.node-red/data/device-plant-mapping.json`

```json
{
  "biodata_12345": {
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
      "notes": "Árbol centenario"
    },
    "created_at": "2024-01-15T10:30:00Z",
    "updated_at": "2024-01-15T10:30:00Z"
  },
  "biodata_67890": {
    "device_id": "biodata_67890",
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
    "created_at": "2024-01-16T09:15:00Z",
    "updated_at": "2024-01-16T09:15:00Z"
  }
}
```

### Implementación en Node-RED

**Función para leer**:
```javascript
const fs = require('fs');
const path = require('path');

const filePath = path.join(process.env.NODE_RED_HOME || '.', 'data', 'device-plant-mapping.json');

// Leer mapeo
function readMapping() {
    try {
        const data = fs.readFileSync(filePath, 'utf8');
        return JSON.parse(data);
    } catch (err) {
        // Si no existe, crear estructura vacía
        return {};
    }
}

// Guardar mapeo
function saveMapping(mapping) {
    fs.writeFileSync(filePath, JSON.stringify(mapping, null, 2), 'utf8');
}

// Consultar por device_id
const mapping = readMapping();
const deviceId = msg.payload.sid || msg.topic.split('/')[1];
const plantInfo = mapping[deviceId] || null;

msg.plantMapping = plantInfo;
return msg;
```

**Función para escribir**:
```javascript
const fs = require('fs');
const path = require('path');

const filePath = path.join(process.env.NODE_RED_HOME || '.', 'data', 'device-plant-mapping.json');

function readMapping() {
    try {
        const data = fs.readFileSync(filePath, 'utf8');
        return JSON.parse(data);
    } catch (err) {
        return {};
    }
}

function saveMapping(mapping) {
    fs.writeFileSync(filePath, JSON.stringify(mapping, null, 2), 'utf8');
}

// Crear o actualizar
const mapping = readMapping();
const deviceId = msg.payload.device_id;
const now = new Date().toISOString();

mapping[deviceId] = {
    device_id: deviceId,
    plant_name: msg.payload.plant_name,
    plant_species: msg.payload.plant_species || null,
    gps_latitude: msg.payload.gps_latitude,
    gps_longitude: msg.payload.gps_longitude,
    gps_altitude: msg.payload.gps_altitude || null,
    additional_data: msg.payload.additional_data || {},
    created_at: mapping[deviceId]?.created_at || now,
    updated_at: now
};

saveMapping(mapping);
msg.payload = { success: true, device_id: deviceId };
return msg;
```

### Modificación manual
Simplemente edita el archivo `device-plant-mapping.json` con cualquier editor de texto. Node-RED leerá los cambios en la próxima consulta.

---

## Opción 2: Google Sheets (⭐ Excelente para gestión visual)

### Ventajas
- ✅ **Interfaz visual**: Editas desde el navegador como una hoja de cálculo
- ✅ **Fácil de compartir**: Puedes dar acceso a otras personas sin conocimientos técnicos
- ✅ **Backup automático**: Google guarda versiones automáticamente
- ✅ **Filtros y búsqueda**: Herramientas nativas de Google Sheets
- ✅ **Gratis** para uso moderado

### Desventajas
- ⚠️ Requiere configuración de API de Google (una vez)
- ⚠️ Límite de llamadas API (pero suficiente para este caso)
- ⚠️ Depende de conexión a internet

### Estructura de la hoja
```
| device_id          | plant_name              | plant_species    | gps_latitude | gps_longitude | gps_altitude | additional_data_json        |
|--------------------|-------------------------|------------------|--------------|---------------|--------------|----------------------------|
| biodata_12345      | Roble del Parque Central| Quercus robur    | 40.4168      | -3.7038       | 650.5        | {"height":12.5,"notes":"..."} |
| biodata_67890      | Pino Mediterráneo       | Pinus halepensis | 40.4175      | -3.7045       | 655.0        | {"height":8.2}              |
```

### Implementación en Node-RED

**Paquete necesario**: `node-red-contrib-google-sheets` o usar Google Sheets API directamente

**Función para leer** (usando Google Sheets API v4):
```javascript
// Configurar: https://developers.google.com/sheets/api/quickstart/nodejs
// Necesitas credenciales OAuth2 o Service Account

const { google } = require('googleapis');
const sheets = google.sheets('v4');

// Leer desde Google Sheets
async function readFromSheets() {
    const auth = new google.auth.GoogleAuth({
        keyFile: 'path/to/service-account-key.json',
        scopes: ['https://www.googleapis.com/auth/spreadsheets.readonly']
    });
    
    const response = await sheets.spreadsheets.values.get({
        auth: auth,
        spreadsheetId: 'TU_SPREADSHEET_ID',
        range: 'Sheet1!A2:G', // Desde fila 2 (saltando encabezados)
    });
    
    const rows = response.data.values;
    const mapping = {};
    
    rows.forEach(row => {
        if (row[0]) { // device_id
            mapping[row[0]] = {
                device_id: row[0],
                plant_name: row[1],
                plant_species: row[2],
                gps_latitude: parseFloat(row[3]),
                gps_longitude: parseFloat(row[4]),
                gps_altitude: row[5] ? parseFloat(row[5]) : null,
                additional_data: row[6] ? JSON.parse(row[6]) : {}
            };
        }
    });
    
    return mapping;
}
```

### Modificación manual
Simplemente abre Google Sheets en el navegador y edita las celdas. Los cambios se reflejan inmediatamente.

---

## Opción 3: Context Global de Node-RED (⭐ Para pruebas rápidas)

### Ventajas
- ✅ **Sin configuración**: Ya está disponible en Node-RED
- ✅ **Muy rápido**: Acceso en memoria
- ✅ **Fácil de usar**: Funciones nativas de Node-RED

### Desventajas
- ⚠️ **Se pierde al reiniciar**: No es persistente (a menos que uses context storage)
- ⚠️ No ideal para producción sin persistencia

### Implementación

**Función para guardar**:
```javascript
// Guardar en context global
const deviceId = msg.payload.device_id;
const plantData = {
    device_id: deviceId,
    plant_name: msg.payload.plant_name,
    plant_species: msg.payload.plant_species,
    gps_latitude: msg.payload.gps_latitude,
    gps_longitude: msg.payload.gps_longitude,
    gps_altitude: msg.payload.gps_altitude,
    additional_data: msg.payload.additional_data || {}
};

context.set(`plant_${deviceId}`, plantData);
// O guardar todo en un objeto:
const allMappings = context.get('plantMappings') || {};
allMappings[deviceId] = plantData;
context.set('plantMappings', allMappings);

return msg;
```

**Función para leer**:
```javascript
const deviceId = msg.payload.sid || msg.topic.split('/')[1];
const allMappings = context.get('plantMappings') || {};
const plantInfo = allMappings[deviceId] || null;

msg.plantMapping = plantInfo;
return msg;
```

### Modificación manual
Puedes usar un nodo `inject` con un payload JSON para actualizar el context, o usar el editor de context de Node-RED (menú → View → Context Data).

---

## Opción 4: SQLite (⭐ Para producción robusta)

### Ventajas
- ✅ **Persistente y confiable**: Base de datos real
- ✅ **Consultas SQL**: Flexibilidad para búsquedas complejas
- ✅ **Transacciones**: Evita corrupción de datos
- ✅ **Escalable**: Maneja muchos registros eficientemente

### Desventajas
- ⚠️ Requiere instalar paquete adicional
- ⚠️ Menos fácil de modificar manualmente (necesitas herramienta SQL)

### Implementación

**Paquete necesario**: `node-red-node-sqlite`

**Estructura de tabla**:
```sql
CREATE TABLE device_plant_mapping (
    device_id TEXT PRIMARY KEY,
    plant_name TEXT NOT NULL,
    plant_species TEXT,
    gps_latitude REAL,
    gps_longitude REAL,
    gps_altitude REAL,
    additional_data TEXT,  -- JSON como texto
    created_at TEXT,
    updated_at TEXT
);
```

**Nodos Node-RED**:
- `sqlite` node para consultas
- Queries: `SELECT * FROM device_plant_mapping WHERE device_id = ?`
- Inserts: `INSERT OR REPLACE INTO device_plant_mapping VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)`

### Modificación manual
Usa herramientas como DB Browser for SQLite o extensiones de VS Code para editar visualmente.

---

## Opción 5: Airtable (⭐ Excelente para equipos no técnicos)

### Ventajas
- ✅ **Interfaz visual muy amigable**: Como Excel pero mejor
- ✅ **API REST simple**: Fácil de integrar
- ✅ **Vistas personalizadas**: Puedes crear diferentes vistas de los mismos datos
- ✅ **Colaboración**: Múltiples usuarios pueden editar simultáneamente
- ✅ **Gratis** hasta cierto límite

### Desventajas
- ⚠️ Requiere cuenta de Airtable (gratis)
- ⚠️ Depende de conexión a internet
- ⚠️ Límite de registros en plan gratuito (1200 registros/base)

### Estructura de tabla en Airtable
```
| device_id (Single line text) | plant_name (Single line text) | plant_species (Single line text) | gps_latitude (Number) | gps_longitude (Number) | gps_altitude (Number) | additional_data (Long text) |
```

### Implementación

**Paquete necesario**: `node-red-contrib-airtable` o HTTP requests directos

**Función para leer** (usando HTTP):
```javascript
const deviceId = msg.payload.sid || msg.topic.split('/')[1];
const airtableBase = 'TU_BASE_ID';
const airtableTable = 'DevicePlantMapping';
const airtableApiKey = 'TU_API_KEY';

msg.url = `https://api.airtable.com/v0/${airtableBase}/${airtableTable}?filterByFormula={device_id}="${deviceId}"`;
msg.headers = {
    'Authorization': `Bearer ${airtableApiKey}`
};
msg.method = 'GET';

return msg;
```

### Modificación manual
Simplemente abre Airtable en el navegador y edita como una hoja de cálculo mejorada.

---

## Recomendación por caso de uso

### Para empezar rápido y probar
→ **Opción 1: Archivo JSON** o **Opción 3: Context Global**

### Para gestión visual y colaboración
→ **Opción 2: Google Sheets** o **Opción 5: Airtable**

### Para producción robusta
→ **Opción 4: SQLite**

### Para equipos no técnicos
→ **Opción 5: Airtable** (mejor UX)

---

## Migración entre opciones

Todas las opciones pueden migrarse fácilmente entre sí porque comparten la misma estructura de datos. Puedes crear un script de migración que lea de una fuente y escriba en otra.

### Ejemplo: Migrar de JSON a Google Sheets
```javascript
// Leer JSON
const fs = require('fs');
const mapping = JSON.parse(fs.readFileSync('device-plant-mapping.json'));

// Convertir a formato de filas para Google Sheets
const rows = Object.values(mapping).map(plant => [
    plant.device_id,
    plant.plant_name,
    plant.plant_species || '',
    plant.gps_latitude,
    plant.gps_longitude,
    plant.gps_altitude || '',
    JSON.stringify(plant.additional_data || {})
]);

// Escribir en Google Sheets (usando API)
```

---

## Código de ejemplo completo: Opción JSON (más simple)

### Nodo Function: "Leer mapeo dispositivo-planta"
```javascript
const fs = require('fs');
const path = require('path');

const filePath = path.join(global.get('userDir') || '.', 'device-plant-mapping.json');

function readMapping() {
    try {
        const data = fs.readFileSync(filePath, 'utf8');
        return JSON.parse(data);
    } catch (err) {
        node.warn('No se encontró archivo de mapeo, usando estructura vacía');
        return {};
    }
}

// Extraer device_id del mensaje
let deviceId;
if (msg.payload && msg.payload.sid) {
    deviceId = msg.payload.sid;
} else if (msg.topic) {
    const parts = msg.topic.split('/');
    deviceId = parts[parts.length - 2] || parts[parts.length - 1];
} else {
    deviceId = null;
}

if (!deviceId) {
    node.warn('No se pudo extraer device_id del mensaje');
    msg.plantMapping = null;
    return msg;
}

// Consultar mapeo
const mapping = readMapping();
const plantInfo = mapping[deviceId] || null;

// Añadir información de planta al mensaje
msg.plantMapping = plantInfo;

// Si existe mapeo, añadir tags para InfluxDB
if (plantInfo) {
    msg.tags = msg.tags || {};
    msg.tags.plant_name = plantInfo.plant_name;
    if (plantInfo.plant_species) {
        msg.tags.plant_species = plantInfo.plant_species;
    }
    if (plantInfo.gps_latitude !== undefined) {
        msg.tags.gps_lat = plantInfo.gps_latitude.toString();
    }
    if (plantInfo.gps_longitude !== undefined) {
        msg.tags.gps_lon = plantInfo.gps_longitude.toString();
    }
} else {
    msg.tags = msg.tags || {};
    msg.tags.plant_name = 'unknown';
}

return msg;
```

### Nodo Function: "Guardar mapeo dispositivo-planta" (para endpoints HTTP)
```javascript
const fs = require('fs');
const path = require('path');

const filePath = path.join(global.get('userDir') || '.', 'device-plant-mapping.json');

function readMapping() {
    try {
        const data = fs.readFileSync(filePath, 'utf8');
        return JSON.parse(data);
    } catch (err) {
        return {};
    }
}

function saveMapping(mapping) {
    // Crear directorio si no existe
    const dir = path.dirname(filePath);
    if (!fs.existsSync(dir)) {
        fs.mkdirSync(dir, { recursive: true });
    }
    
    fs.writeFileSync(filePath, JSON.stringify(mapping, null, 2), 'utf8');
}

// Validar datos de entrada
const deviceId = msg.payload.device_id;
const plantName = msg.payload.plant_name;

if (!deviceId || !plantName) {
    msg.statusCode = 400;
    msg.payload = { error: 'device_id y plant_name son requeridos' };
    return msg;
}

// Validar coordenadas GPS si se proporcionan
if (msg.payload.gps_latitude !== undefined) {
    const lat = parseFloat(msg.payload.gps_latitude);
    if (isNaN(lat) || lat < -90 || lat > 90) {
        msg.statusCode = 400;
        msg.payload = { error: 'gps_latitude debe estar entre -90 y 90' };
        return msg;
    }
}

if (msg.payload.gps_longitude !== undefined) {
    const lon = parseFloat(msg.payload.gps_longitude);
    if (isNaN(lon) || lon < -180 || lon > 180) {
        msg.statusCode = 400;
        msg.payload = { error: 'gps_longitude debe estar entre -180 y 180' };
        return msg;
    }
}

// Leer mapeo actual
const mapping = readMapping();
const now = new Date().toISOString();

// Crear o actualizar registro
mapping[deviceId] = {
    device_id: deviceId,
    plant_name: plantName,
    plant_species: msg.payload.plant_species || null,
    gps_latitude: msg.payload.gps_latitude !== undefined ? parseFloat(msg.payload.gps_latitude) : null,
    gps_longitude: msg.payload.gps_longitude !== undefined ? parseFloat(msg.payload.gps_longitude) : null,
    gps_altitude: msg.payload.gps_altitude !== undefined ? parseFloat(msg.payload.gps_altitude) : null,
    additional_data: msg.payload.additional_data || {},
    created_at: mapping[deviceId]?.created_at || now,
    updated_at: now
};

// Guardar
saveMapping(mapping);

msg.statusCode = 200;
msg.payload = { 
    success: true, 
    device_id: deviceId,
    plant_name: plantName,
    message: 'Mapeo guardado correctamente'
};

return msg;
```


