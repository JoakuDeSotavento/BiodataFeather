# 🔐 Configuración Segura de Credenciales

## ⚠️ IMPORTANTE - Seguridad

**NUNCA** subas archivos con credenciales reales al repositorio. Esto incluye:
- Contraseñas de WiFi
- Tokens de MQTT
- Claves de API
- URLs de servicios privados

## 📋 Pasos para Configurar Credenciales

### 1. Crear archivo de credenciales
```bash
# Copia el archivo de ejemplo
cp secrets_example.h secrets.h
```

### 2. Editar credenciales
Abre `secrets.h` y cambia los valores:

```cpp
// WiFi
#define WIFI_SSID "tu_red_wifi_real"
#define WIFI_PASSWORD "tu_password_real"

// MQTT
#define MQTT_BROKER "tu_broker_mqtt.com"
#define MQTT_PORT 1883
#define MQTT_USERNAME "tu_usuario"
#define MQTT_PASSWORD "tu_password"
```

### 3. Verificar que está en .gitignore
El archivo `secrets.h` debe estar en `.gitignore` para no subirse al repositorio.

## 🔧 Configuraciones Comunes

### Node-RED Local
```cpp
#define MQTT_BROKER "192.168.1.100"
#define MQTT_PORT 1883
#define MQTT_USERNAME ""  // vacío si no requiere autenticación
#define MQTT_PASSWORD ""
```

### HiveMQ (Gratuito)
```cpp
#define MQTT_BROKER "broker.hivemq.com"
#define MQTT_PORT 1883
#define MQTT_USERNAME ""  // no requiere autenticación
#define MQTT_PASSWORD ""
```

### Mosquitto Local
```cpp
#define MQTT_BROKER "localhost"
#define MQTT_PORT 1883
#define MQTT_USERNAME "usuario"
#define MQTT_PASSWORD "password"
```

### AWS IoT
```cpp
#define MQTT_BROKER "tu-endpoint.iot.region.amazonaws.com"
#define MQTT_PORT 8883
#define MQTT_USERNAME ""  // usar certificados
#define MQTT_PASSWORD ""
```

## 🛡️ Buenas Prácticas de Seguridad

### 1. Usar Variables de Entorno (Recomendado)
```cpp
// En lugar de hardcodear credenciales
#define WIFI_SSID getenv("WIFI_SSID")
#define WIFI_PASSWORD getenv("WIFI_PASSWORD")
```

### 2. Encriptar Credenciales
```cpp
// Usar funciones de encriptación
#define WIFI_PASSWORD decrypt("encrypted_password")
```

### 3. Rotar Credenciales Regularmente
- Cambia contraseñas cada 3-6 meses
- Usa contraseñas fuertes
- No reutilices contraseñas

### 4. Monitorear Acceso
- Revisa logs de acceso
- Configura alertas de acceso no autorizado
- Usa autenticación de dos factores cuando sea posible

## 🚨 Si Accidentalmente Subiste Credenciales

### 1. Cambiar Credenciales Inmediatamente
- Cambia todas las contraseñas expuestas
- Revoca tokens de API
- Actualiza claves de acceso

### 2. Limpiar Historial de Git
```bash
# Eliminar archivo del historial
git filter-branch --force --index-filter \
'git rm --cached --ignore-unmatch **/secrets.h' \
--prune-empty --tag-name-filter cat -- --all

# Forzar push
git push origin --force --all
```

### 3. Verificar que se Eliminó
```bash
# Buscar en el historial
git log --all --full-history -- **/secrets.h
```

## 📞 Soporte

Si tienes problemas con la configuración:
1. Verifica que el archivo `secrets.h` existe
2. Comprueba que las credenciales son correctas
3. Verifica conectividad de red
4. Revisa logs de error en el serial monitor

## 🔍 Verificación

Para verificar que todo está configurado correctamente:

1. **Compila el proyecto** - No debe haber errores
2. **Sube al ESP32** - Debe conectarse a WiFi
3. **Verifica en serial** - Debe mostrar "WiFi conectado"
4. **Prueba MQTT** - Debe mostrar "MQTT conectado"

## 📝 Notas Adicionales

- Mantén una copia de seguridad segura de tus credenciales
- Usa un gestor de contraseñas
- Documenta cambios en credenciales
- Considera usar certificados en lugar de contraseñas para MQTT
