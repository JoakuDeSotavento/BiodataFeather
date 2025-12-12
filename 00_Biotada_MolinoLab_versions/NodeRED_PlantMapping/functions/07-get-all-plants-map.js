/**
 * Función Node-RED: Obtener todas las plantas activas para mapa
 * 
 * Endpoint: GET /api/plants/map
 * 
 * Retorna solo las asociaciones activas con coordenadas GPS para visualización en mapa
 */

const fs = require('fs');
const path = require('path');

const userDir = global.get('userDir') || process.env.NODE_RED_HOME || '.';
const filePath = path.join(userDir, 'data', 'device-plant-mapping.json');

// Leer archivo
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

// Buscar todas las asociaciones activas
const now = new Date().toISOString();
const activeAssociations = data.associations.filter(assoc => {
    return assoc.start_time <= now &&
           (assoc.end_time === null || assoc.end_time > now) &&
           assoc.gps_latitude !== null &&
           assoc.gps_longitude !== null;
});

// Si hay múltiples asociaciones activas para el mismo dispositivo, usar la más reciente
const deviceMap = new Map();
activeAssociations.forEach(assoc => {
    if (!deviceMap.has(assoc.device_id)) {
        deviceMap.set(assoc.device_id, assoc);
    } else {
        const existing = deviceMap.get(assoc.device_id);
        if (new Date(assoc.start_time) > new Date(existing.start_time)) {
            deviceMap.set(assoc.device_id, assoc);
        }
    }
});

// Convertir a array y formatear para mapa
const plants = Array.from(deviceMap.values()).map(assoc => ({
    device_id: assoc.device_id,
    association_id: assoc.id,
    plant_name: assoc.plant_name,
    plant_species: assoc.plant_species,
    gps_latitude: assoc.gps_latitude,
    gps_longitude: assoc.gps_longitude,
    gps_altitude: assoc.gps_altitude,
    start_time: assoc.start_time,
    end_time: assoc.end_time
}));

msg.statusCode = 200;
msg.payload = {
    plants: plants,
    count: plants.length
};

return msg;

