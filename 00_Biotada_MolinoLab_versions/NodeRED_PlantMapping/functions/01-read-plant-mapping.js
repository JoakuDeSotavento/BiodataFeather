/**
 * Función Node-RED: Leer archivo JSON y buscar asociación activa
 * 
 * Esta función lee el archivo device-plant-mapping.json y busca la asociación
 * activa para un device_id dado en el momento actual.
 * 
 * Entrada: msg debe contener device_id en msg.device_id o se extraerá del topic/payload
 * Salida: msg.plantMapping con la asociación activa o null
 */

const fs = require('fs');
const path = require('path');

// Obtener directorio de usuario de Node-RED
const userDir = global.get('userDir') || process.env.NODE_RED_HOME || '.';
const filePath = path.join(userDir, 'data', 'device-plant-mapping.json');

// Caché en memoria con TTL de 60 segundos
const CACHE_TTL = 60000; // 60 segundos
let cache = {
    data: null,
    timestamp: 0
};

/**
 * Leer archivo JSON con caché
 */
function readMappingFile() {
    const now = Date.now();
    
    // Si el caché es válido, usarlo
    if (cache.data && (now - cache.timestamp) < CACHE_TTL) {
        return cache.data;
    }
    
    // Leer archivo
    let data = { associations: [] };
    try {
        if (fs.existsSync(filePath)) {
            const fileData = fs.readFileSync(filePath, 'utf8');
            data = JSON.parse(fileData);
        } else {
            // Crear archivo si no existe
            fs.mkdirSync(path.dirname(filePath), { recursive: true });
            fs.writeFileSync(filePath, JSON.stringify(data, null, 2), 'utf8');
            node.warn('Archivo device-plant-mapping.json creado en: ' + filePath);
        }
    } catch (err) {
        node.error('Error al leer archivo de mapeo: ' + err.message);
        data = { associations: [] };
    }
    
    // Actualizar caché
    cache.data = data;
    cache.timestamp = now;
    
    return data;
}

/**
 * Buscar asociación activa para un device_id
 */
function findActiveAssociation(deviceId, timestamp) {
    const data = readMappingFile();
    const now = timestamp || new Date().toISOString();
    
    // Filtrar asociaciones activas
    const activeAssociations = data.associations.filter(assoc => {
        return assoc.device_id === deviceId &&
               assoc.start_time <= now &&
               (assoc.end_time === null || assoc.end_time > now);
    });
    
    // Si hay múltiples asociaciones activas, usar la más reciente
    if (activeAssociations.length > 0) {
        activeAssociations.sort((a, b) => new Date(b.start_time) - new Date(a.start_time));
        return activeAssociations[0];
    }
    
    return null;
}

// Extraer device_id del mensaje
let deviceId = msg.device_id;

if (!deviceId) {
    // Intentar extraer del payload JSON
    if (msg.payload && typeof msg.payload === 'object') {
        // Buscar sensor_id o sid en el payload
        deviceId = msg.payload.sensor_id || msg.payload.sid;
    }
    // Intentar extraer del topic MQTT
    if (!deviceId && msg.topic) {
        const parts = msg.topic.split('/');
        // Patrón esperado: biodata/{device_id}/midi o {base}/{device_id}
        // El device_id está en la posición 1 (índice 1) después del base topic
        if (parts.length >= 2) {
            deviceId = parts[parts.length - 2]; // Penúltimo elemento (antes de "midi")
        }
        if (!deviceId && parts.length >= 1) {
            deviceId = parts[parts.length - 1]; // Último elemento como fallback
        }
    }
}

if (!deviceId) {
    node.warn('No se pudo extraer device_id del mensaje');
    msg.plantMapping = null;
    return msg;
}

// Buscar asociación activa
const plantMapping = findActiveAssociation(deviceId);

msg.device_id = deviceId;
msg.plantMapping = plantMapping;

return msg;

