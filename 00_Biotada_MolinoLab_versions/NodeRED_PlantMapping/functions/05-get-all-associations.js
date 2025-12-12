/**
 * Función Node-RED: Obtener todas las asociaciones de un dispositivo
 * 
 * Endpoint: GET /device-plant/associations/:device_id
 * 
 * Retorna todas las asociaciones (activas e históricas) de un dispositivo
 */

const fs = require('fs');
const path = require('path');

const userDir = global.get('userDir') || process.env.NODE_RED_HOME || '.';
const filePath = path.join(userDir, 'data', 'device-plant-mapping.json');

// Extraer device_id de la URL (req.params.device_id)
const deviceId = msg.req.params.device_id || msg.device_id;

if (!deviceId) {
    msg.statusCode = 400;
    msg.payload = { error: 'device_id es requerido' };
    return msg;
}

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

// Filtrar asociaciones del dispositivo
const deviceAssociations = data.associations.filter(assoc => assoc.device_id === deviceId);

// Ordenar por start_time descendente (más recientes primero)
deviceAssociations.sort((a, b) => new Date(b.start_time) - new Date(a.start_time));

msg.statusCode = 200;
msg.payload = {
    device_id: deviceId,
    count: deviceAssociations.length,
    associations: deviceAssociations
};

return msg;

