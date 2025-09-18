# 🚀 INICIO RÁPIDO - BiodataFeather Integrated v1.0

## Configuración en 5 minutos

### 1. 📁 Preparar el proyecto
- Abre Arduino IDE
- Ve a `Archivo > Abrir carpeta`
- Selecciona la carpeta `Biodata_Feather_ESP32_Integrated_v1`

### 2. 🔧 Configurar credenciales
Edita el archivo `secrets.h` (solo información sensible):
```cpp
#define WIFI_SSID "tu_wifi_ssid"
#define WIFI_PASSWORD "tu_wifi_password"
#define MQTT_BROKER "tu_mqtt_broker.com"  // o IP local
#define MQTT_PORT 1883
```

Para múltiples dispositivos, cambia `SENSOR_ID` en el archivo principal:
```cpp
#define SENSOR_ID "biodata1" // Cambia a biodata2, biodata3, etc.
```

### 3. 📚 Instalar librerías
Ejecuta estos comandos en Arduino IDE:
```
Herramientas > Administrar Bibliotecas
```
Busca e instala:
- `PubSubClient` (Nick O'Leary)
- `ArduinoJson` (Benoit Blanchon)
- `Adafruit TinyUSB Library`

### 4. 🔌 Conectar hardware
```
BME688:
- VCC → 3.3V
- GND → GND  
- SDA → Pin 21
- SCL → Pin 20

LTR329:
- VCC → 3.3V
- GND → GND
- SDA → Pin 21
- SCL → Pin 20

Galvanómetro:
- Señal → Pin 44
- GND → GND
```

### 5. ⚡ Compilar y subir
- Selecciona tu placa ESP32-S3 Feather
- Compila y sube el código
- Abre el Monitor Serial (115200 baud)

### 6. 🎯 Verificar funcionamiento
Deberías ver:
```
BiodataFeather Integrated v1.0 iniciado
Sensores + MIDI + MQTT activos
LTR329 OK
BME68x OK
WiFi conectado
MQTT conectado
```

### 7. 📊 Configurar Node-RED (opcional)
- Instala Node-RED
- Importa el archivo `NodeRED_Flow.json`
- Configura el broker MQTT
- Accede al dashboard en `http://localhost:1880/ui`

## 🆘 Solución de problemas

### Error de compilación
- Verifica que todas las librerías estén instaladas
- Asegúrate de tener el board ESP32-S3 correcto

### Sensores no detectados
- Verifica conexiones I2C
- Comprueba voltaje en VCC (3.3V)

### WiFi no conecta
- Verifica credenciales en `secrets.h`
- Comprueba que la red esté disponible

### MQTT no conecta
- Verifica que el broker esté funcionando
- Comprueba puerto y credenciales

## 📞 Soporte
- Revisa el `README.md` completo
- Consulta `libraries.txt` para librerías
- Verifica conexiones con multímetro

¡Listo! Tu BiodataFeather Integrated está funcionando 🎉
