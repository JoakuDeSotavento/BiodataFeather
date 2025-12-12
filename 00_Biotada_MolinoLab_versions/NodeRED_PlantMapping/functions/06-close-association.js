/**
 * Función Node-RED: Cerrar asociación activa de un dispositivo
 * 
 * Endpoint: POST /device-plant/close/:device_id
 * 
 * Body opcional:
 * {
 *   "end_time": "2024-01-23T13:00:00Z" (opcional, por defecto ahora)
 * }
 */

const fs = require('fs');
const path = require('path');

const userDir = global.get('userDir') || process.env.NODE_RED_HOME || '.';
const filePath = path.join(userDir, 'data', 'device-plant-mapping.json');

// Extraer device_id de la URL
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
const endTime = msg.payload.end_time || now;

const activeAssociations = data.associations.filter(assoc => {
    return assoc.device_id === deviceId &&
           assoc.end_time === null &&
           assoc.start_time <= endTime;
});

if (activeAssociations.length === 0) {
    msg.statusCode = 404;
    msg.payload = { error: 'No se encontró asociación activa para cerrar' };
    return msg;
}

// Ordenar por start_time descendente y tomar la más reciente
activeAssociations.sort((a, b) => new Date(b.start_time) - new Date(a.start_time));
const activeAssociation = activeAssociations[0];

// Validar que end_time sea mayor que start_time
if (endTime <= activeAssociation.start_time) {
    msg.statusCode = 400;
    msg.payload = { error: 'end_time debe ser mayor que start_time' };
    return msg;
}

// Cerrar asociación
const index = data.associations.findIndex(a => a.id === activeAssociation.id);
if (index !== -1) {
    data.associations[index].end_time = endTime;
    data.associations[index].updated_at = now;
}

// Escribir archivo
try {
    fs.writeFileSync(filePath, JSON.stringify(data, null, 2), 'utf8');
    
    msg.statusCode = 200;
    msg.payload = {
        success: true,
        message: 'Asociación cerrada correctamente',
        association: data.associations[index]
    };
} catch (err) {
    node.error('Error al escribir archivo: ' + err.message);
    msg.statusCode = 500;
    msg.payload = { error: 'Error al guardar cambios' };
}

return msg;

