Líneas de acción sugeridas
Revisar primero la configuración de la placa en el IDE (TinyUSB en modo “CDC+MIDI”, PID/VID correctos, USB CDC al arranque). Entre ambas ramas no hay cambios reales en la lógica de USBMIDI, así que es muy probable que la versión “cleanstart” se haya compilado con opciones por defecto que no exponen el interface MIDI al host.
Comprobar en tiempo de ejecución que USB.begin() y usbMIDI.begin() se están ejecutando: añade trazas temporales justo después del if (usbmidi) para descartar que el flag haya quedado a 0 por datos viejos en EEPROM o por algún refactor.
Main.inoLines 13-17
  if(usbmidi) {    USB.begin(); // Inicializar USB (ESP32 v3.3.2)    usbMIDI.begin(); // Inicializar USB-MIDI nativo  }
Probar a deshabilitar temporalmente el nuevo drenaje del buffer MQTT (comentando flushMQTTPayload(); y/o ENABLE_RAW_LOGGING) para ver si la sobrecarga extra en el loop() está bloqueando el stack TinyUSB durante el arranque.
Main.inoLines 135-142
  if(bufferEnabled) {    ...    flushMQTTPayload();  }
Medir memoria libre tras inicializar el buffer MQTT: el nuevo DynamicJsonDocument con bloques crudos incrementa mucho la huella; si el heap queda bajo, TinyUSB puede quedar inestable.
MQTTInflux.inoLines 233-342
  const size_t capacity = ...  DynamicJsonDocument doc(capacity);
Alinear versiones de núcleo/librerías: asegúrate de estar usando el mismo release de Arduino-ESP32 y de las libs USB que compilaban el sketch “MQTT_midi_working”; un salto de versión puede haber cambiado el soporte a USBMIDI.
Confirmar que el cambio en el UART hardware (Serial1 ahora fuerza el pin 10 para TX) no está reutilizando líneas compartidas con USB o provocando consumo adicional inesperado.
Biodata_Feather_ESP32_07.inoLines 387-389
  const int MIDI_TX_PIN = 10;  // GPIO10 conectado al minijack MIDI  Serial1.begin(31250, SERIAL_8N1, -1, MIDI_TX_PIN);
Próximos pasos: validar cada punto en orden hasta que el host vuelva a reconocer el dispositivo MIDI; documenta qué cambio concreta el problema antes de mezclar de nuevo.