/**
 * Función Node-RED: Crear nueva asociación dispositivo-planta
 * 
 * Endpoint: POST /device-plant/associate
 * 
 * Body esperado:
 * {
 *   "device_id": "biodata_12345",
 *   "plant_name": "Roble del Parque Central",
 *   "plant_species": "Quercus robur" (opcional),
 *   "gps_latitude": 40.4168,
 *   "gps_longitude": -3.7038,
 *   "gps_altitude": 650.5 (opcional),
 *   "start_time": "2024-01-15T08:00:00Z" (opcional, por defecto ahora),
 *   "end_time": "2024-01-22T08:00:00Z" (opcional, null para activa),
 *   "additional_data": {} (opcional)
 * }
 */

const fs = require('fs');
const path = require('path');
const crypto = require('crypto');

const userDir = global.get('userDir') || process.env.NODE_RED_HOME || '.';
const filePath = path.join(userDir, 'data', 'device-plant-mapping.json');

// Leer datos actuales
let data = { associations: [] };
try {
    if (fs.existsSync(filePath)) {
        const fileData = fs.readFileSync(filePath, 'utf8');
        data = JSON.parse(fileData);
    }
} catch (err) {
    node.error('Error al leer archivo: ' + err.message);
    msg.statusCode = 500;
    msg.payload = { error: 'Error al leer archivo de mapeo' };
    return msg;
}

// Validar datos de entrada
const payload = msg.payload;
const deviceId = payload.device_id;
const plantName = payload.plant_name;

if (!deviceId || !plantName) {
    msg.statusCode = 400;
    msg.payload = { error: 'device_id y plant_name son requeridos' };
    return msg;
}

// Validar coordenadas GPS si se proporcionan
if (payload.gps_latitude !== undefined) {
    const lat = parseFloat(payload.gps_latitude);
    if (isNaN(lat) || lat < -90 || lat > 90) {
        msg.statusCode = 400;
        msg.payload = { error: 'gps_latitude debe estar entre -90 y 90' };
        return msg;
    }
}

if (payload.gps_longitude !== undefined) {
    const lon = parseFloat(payload.gps_longitude);
    if (isNaN(lon) || lon < -180 || lon > 180) {
        msg.statusCode = 400;
        msg.payload = { error: 'gps_longitude debe estar entre -180 y 180' };
        return msg;
    }
}

// Generar ID único para la asociación
const now = new Date().toISOString();
const associationId = 'assoc_' + Date.now() + '_' + crypto.randomBytes(4).toString('hex');

// Timestamps
const startTime = payload.start_time || now;
let endTime = payload.end_time !== undefined ? payload.end_time : null;

// Validar que end_time sea mayor que start_time si ambos están presentes
if (endTime !== null && endTime <= startTime) {
    msg.statusCode = 400;
    msg.payload = { error: 'end_time debe ser mayor que start_time' };
    return msg;
}

// Crear nueva asociación
const newAssociation = {
    id: associationId,
    device_id: deviceId,
    plant_name: plantName,
    plant_species: payload.plant_species || null,
    gps_latitude: payload.gps_latitude !== undefined ? parseFloat(payload.gps_latitude) : null,
    gps_longitude: payload.gps_longitude !== undefined ? parseFloat(payload.gps_longitude) : null,
    gps_altitude: payload.gps_altitude !== undefined ? parseFloat(payload.gps_altitude) : null,
    additional_data: payload.additional_data || {},
    start_time: startTime,
    end_time: endTime,
    created_at: now,
    updated_at: now
};

// (Opcional) Cerrar asociación activa anterior del mismo dispositivo
// Solo si la nueva asociación tiene start_time y la anterior está activa
if (endTime === null) {
    const activeAssociations = data.associations.filter(assoc => {
        return assoc.device_id === deviceId &&
               assoc.end_time === null &&
               assoc.start_time <= startTime;
    });
    
    if (activeAssociations.length > 0) {
        // Cerrar la asociación activa más reciente
        activeAssociations.sort((a, b) => new Date(b.start_time) - new Date(a.start_time));
        const activeAssoc = activeAssociations[0];
        const index = data.associations.findIndex(a => a.id === activeAssoc.id);
        if (index !== -1) {
            data.associations[index].end_time = startTime;
            data.associations[index].updated_at = now;
        }
    }
}

// Añadir nueva asociación
data.associations.push(newAssociation);

// Escribir archivo
try {
    // Crear directorio si no existe
    const dir = path.dirname(filePath);
    if (!fs.existsSync(dir)) {
        fs.mkdirSync(dir, { recursive: true });
    }
    
    fs.writeFileSync(filePath, JSON.stringify(data, null, 2), 'utf8');
    
    // Invalidar caché (si existe en otra función)
    // Esto se manejará automáticamente con el TTL del caché
    
    msg.statusCode = 201;
    msg.payload = {
        success: true,
        association_id: associationId,
        message: 'Asociación creada correctamente',
        association: newAssociation
    };
} catch (err) {
    node.error('Error al escribir archivo: ' + err.message);
    msg.statusCode = 500;
    msg.payload = { error: 'Error al guardar asociación' };
}

return msg;

