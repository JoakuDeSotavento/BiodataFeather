/**
 * Función Node-RED: Enriquecer mensaje con tags de planta para InfluxDB
 * 
 * Esta función añade tags de planta al mensaje basándose en la asociación activa
 * encontrada en msg.plantMapping.
 * 
 * Entrada: msg debe contener msg.plantMapping (objeto o null)
 * Salida: msg con tags añadidos para InfluxDB
 */

// Verificar si existe plantMapping
if (!msg.plantMapping) {
    // No hay asociación activa, usar tag "unknown"
    msg.tags = msg.tags || {};
    msg.tags.plant_name = "unknown";
    return msg;
}

const plantMapping = msg.plantMapping;

// Inicializar tags si no existen
msg.tags = msg.tags || {};

// Añadir tags de planta
msg.tags.plant_name = plantMapping.plant_name;

if (plantMapping.plant_species) {
    msg.tags.plant_species = plantMapping.plant_species;
}

if (plantMapping.gps_latitude !== undefined && plantMapping.gps_latitude !== null) {
    msg.tags.gps_lat = plantMapping.gps_latitude.toString();
}

if (plantMapping.gps_longitude !== undefined && plantMapping.gps_longitude !== null) {
    msg.tags.gps_lon = plantMapping.gps_longitude.toString();
}

// Opcional: añadir ID de asociación para trazabilidad
if (plantMapping.id) {
    msg.tags.association_id = plantMapping.id;
}

return msg;

