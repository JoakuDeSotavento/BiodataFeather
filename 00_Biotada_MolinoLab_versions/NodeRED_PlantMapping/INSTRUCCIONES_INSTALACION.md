# Instrucciones de Instalación - Node-RED Plant Mapping

## 📋 Requisitos Previos

- Node-RED instalado y funcionando
- Acceso al editor de Node-RED (normalmente `http://localhost:1880`)
- Flujo MQTT → InfluxDB existente

## 🚀 Instalación Paso a Paso

### Paso 1: Preparar el archivo de datos

1. **Ubica tu directorio de datos de Node-RED**:
   - **Linux/Mac**: `~/.node-red/data/`
   - **Windows**: `C:\Users\{tu_usuario}\.node-red\data\`

2. **Crea el archivo `device-plant-mapping.json`** en ese directorio con este contenido:

```json
{
  "associations": []
}
```

**O simplemente copia** el archivo `data/device-plant-mapping.json` de este directorio a `~/.node-red/data/`

### Paso 2: Crear las funciones en Node-RED

Para cada función, sigue estos pasos:

1. Abre Node-RED en tu navegador
2. Haz clic en el menú (☰) → **Manage palette** → **Nodes**
3. Busca y asegúrate de tener instalado el nodo **Function** (viene por defecto)
4. Arrastra un nodo **Function** al workspace
5. Haz doble clic en el nodo para editarlo
6. Copia el código del archivo correspondiente de la carpeta `functions/`
7. Pega el código en el editor
8. Dale un nombre descriptivo
9. Haz clic en **Done**

#### Lista de funciones a crear:

| Nombre del Nodo | Archivo | Descripción |
|----------------|---------|-------------|
| `Leer mapeo planta` | `01-read-plant-mapping.js` | Lee el archivo JSON y busca asociación activa |
| `Añadir tags planta` | `02-enrich-influxdb-tags.js` | Añade tags de planta al mensaje para InfluxDB |
| `Crear asociación` | `03-create-association.js` | Crea una nueva asociación dispositivo-planta |
| `Obtener asociación activa` | `04-get-active-association.js` | Obtiene la asociación activa de un dispositivo |
| `Obtener todas asociaciones` | `05-get-all-associations.js` | Obtiene todas las asociaciones de un dispositivo |
| `Cerrar asociación` | `06-close-association.js` | Cierra una asociación activa |
| `Obtener plantas mapa` | `07-get-all-plants-map.js` | Obtiene todas las plantas activas para mapa |

### Paso 3: Integrar en tu flujo MQTT → InfluxDB existente

**IMPORTANTE**: Este paso modifica tu flujo existente. Haz una copia de seguridad primero.

1. **Localiza tu nodo MQTT In** que recibe mensajes de los dispositivos
   - Busca el nodo que tiene un topic como `biodata/+/midi` o similar

2. **Añade dos nodos Function después del nodo MQTT In**:
   - Arrastra un nodo **Function** y nómbralo `Leer mapeo planta`
   - Arrastra otro nodo **Function** y nómbralo `Añadir tags planta`
   - Copia el código correspondiente en cada uno

3. **Conecta los nodos**:
   ```
   MQTT In → Leer mapeo planta → Añadir tags planta → [tu nodo InfluxDB Out existente]
   ```

4. **Configura el nodo "Leer mapeo planta"**:
   - Asegúrate de que tiene el código de `01-read-plant-mapping.js`
   - No necesita configuración adicional

5. **Configura el nodo "Añadir tags planta"**:
   - Asegúrate de que tiene el código de `02-enrich-influxdb-tags.js`
   - No necesita configuración adicional

6. **Despliega el flujo**: Haz clic en el botón **Deploy** (arriba a la derecha)

### Paso 4: Crear endpoints HTTP para gestión

Crea los siguientes flujos para gestionar las asociaciones:

#### Endpoint 1: Crear asociación

1. Arrastra un nodo **http in**
2. Configúralo:
   - **Method**: `POST`
   - **URL**: `/device-plant/associate`
3. Conecta un nodo **Function** con el código de `03-create-association.js`
4. Conecta un nodo **http response**
5. Conecta: `http in` → `Crear asociación` → `http response`

#### Endpoint 2: Obtener asociación activa

1. Arrastra un nodo **http in**
2. Configúralo:
   - **Method**: `GET`
   - **URL**: `/device-plant/active/:device_id`
3. Conecta un nodo **Function** con el código de `04-get-active-association.js`
4. Conecta un nodo **http response**
5. Conecta: `http in` → `Obtener asociación activa` → `http response`

#### Endpoint 3: Obtener todas las asociaciones

1. Arrastra un nodo **http in**
2. Configúralo:
   - **Method**: `GET`
   - **URL**: `/device-plant/associations/:device_id`
3. Conecta un nodo **Function** con el código de `05-get-all-associations.js`
4. Conecta un nodo **http response**
5. Conecta: `http in` → `Obtener todas asociaciones` → `http response`

#### Endpoint 4: Cerrar asociación

1. Arrastra un nodo **http in**
2. Configúralo:
   - **Method**: `POST`
   - **URL**: `/device-plant/close/:device_id`
3. Conecta un nodo **Function** con el código de `06-close-association.js`
4. Conecta un nodo **http response**
5. Conecta: `http in` → `Cerrar asociación` → `http response`

#### Endpoint 5: Obtener plantas para mapa

1. Arrastra un nodo **http in**
2. Configúralo:
   - **Method**: `GET`
   - **URL**: `/api/plants/map`
3. Conecta un nodo **Function** con el código de `07-get-all-plants-map.js`
4. Conecta un nodo **http response**
5. Conecta: `http in` → `Obtener plantas mapa` → `http response`

### Paso 5: Probar la instalación

1. **Crea una asociación de prueba**:
   ```bash
   curl -X POST http://localhost:1880/device-plant/associate \
     -H "Content-Type: application/json" \
     -d '{
       "device_id": "biodata_test",
       "plant_name": "Planta de Prueba",
       "gps_latitude": 40.4168,
       "gps_longitude": -3.7038
     }'
   ```

2. **Verifica que se creó**:
   ```bash
   curl http://localhost:1880/device-plant/active/biodata_test
   ```

3. **Envía un mensaje MQTT de prueba** y verifica que los tags se añaden correctamente en InfluxDB

## 🔍 Verificación

### Verificar que el archivo se crea correctamente

1. Ve a `~/.node-red/data/device-plant-mapping.json`
2. Debe existir y tener la estructura `{"associations": []}`

### Verificar que las funciones funcionan

1. En Node-RED, abre el **Debug** panel (lateral derecho)
2. Añade un nodo **Debug** después de `Leer mapeo planta`
3. Deberías ver en el debug:
   - `msg.device_id`: El ID del dispositivo
   - `msg.plantMapping`: La asociación activa o `null`

### Verificar que los tags se añaden

1. Añade un nodo **Debug** después de `Añadir tags planta`
2. Deberías ver:
   - `msg.tags.plant_name`: Nombre de la planta o "unknown"
   - `msg.tags.plant_species`: Especie (si existe)
   - `msg.tags.gps_lat` y `msg.tags.gps_lon`: Coordenadas (si existen)

## ⚠️ Solución de Problemas

### Error: "No se pudo extraer device_id"

**Causa**: El formato del mensaje MQTT no coincide con lo esperado.

**Solución**:
1. Añade un nodo **Debug** después del nodo MQTT In
2. Verifica el formato de `msg.topic` y `msg.payload`
3. Ajusta la función `01-read-plant-mapping.js` si es necesario

### Error: "Error al leer archivo"

**Causa**: Permisos o ruta incorrecta.

**Solución**:
1. Verifica que el archivo existe en `~/.node-red/data/device-plant-mapping.json`
2. Verifica permisos de lectura/escritura
3. Verifica que `global.get('userDir')` retorna la ruta correcta

### El tag siempre es "unknown"

**Causa**: No hay asociación activa para el dispositivo.

**Solución**:
1. Crea una asociación usando el endpoint `/device-plant/associate`
2. Verifica que `start_time` sea menor o igual a ahora
3. Verifica que `end_time` sea `null` o mayor que ahora

## 📝 Notas Adicionales

- Las funciones usan caché en memoria (60 segundos) para optimizar lecturas
- Si modificas el archivo JSON manualmente, los cambios se reflejarán después de que expire el caché
- Para invalidar el caché inmediatamente, reinicia Node-RED

## 🆘 ¿Necesitas ayuda?

Si encuentras problemas:
1. Revisa los logs de Node-RED (consola donde está corriendo)
2. Verifica el panel Debug de Node-RED
3. Comprueba que todos los archivos están en las ubicaciones correctas

