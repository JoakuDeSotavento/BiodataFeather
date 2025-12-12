/**
 * Función Node-RED: Obtener asociación activa de un dispositivo
 * 
 * Endpoint: GET /device-plant/active/:device_id
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

// Buscar asociación activa
const now = new Date().toISOString();
const activeAssociations = data.associations.filter(assoc => {
    return assoc.device_id === deviceId &&
           assoc.start_time <= now &&
           (assoc.end_time === null || assoc.end_time > now);
});

if (activeAssociations.length === 0) {
    msg.statusCode = 404;
    msg.payload = { error: 'No se encontró asociación activa para el dispositivo' };
    return msg;
}

// Ordenar por start_time descendente y tomar la más reciente
activeAssociations.sort((a, b) => new Date(b.start_time) - new Date(a.start_time));
const activeAssociation = activeAssociations[0];

msg.statusCode = 200;
msg.payload = activeAssociation;

return msg;

