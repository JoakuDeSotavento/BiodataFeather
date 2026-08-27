// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void setup() {
  ensureDeviceIdentity();

  //load from EEPROM memory
  EEPROM.begin(EEPROM_SIZE);

  // Same order as v10: CDC first, then register MIDI, then start TinyUSB.
  // USB.begin() before Serial.begin() starts a MIDI-only stack; Serial.begin()
  // afterwards either misses CDC or re-enumerates and drops the MIDI interface.
  if (debugSerial || rawSerial) {
    Serial.begin(115200); // Serial baud for debugging and raw
  }

  if (usbmidi) {
    static char usbProduct[24];
    snprintf(usbProduct, sizeof(usbProduct), "Biodata %s", deviceSuffix.c_str());
    USB.productName(usbProduct);
    usbMIDI.begin();
    delay(500);
    USB.begin();
  }

  if (debugSerial || rawSerial) {
    delay(1000); // I2C stabilization after USB/Serial are up
  }
  
  //pinMode(buttonPin, INPUT_PULLUP); //button managed by PinButton
  pinMode(interruptPin, INPUT_PULLUP); //pulse input

  button.begin();
 
//check if button is held at startup, potentially for reset stuff
 

    //LED light show
    //ledcSetup(0,5000,13);
    for(byte i=0;i<5;i++) {
        ledFaders[i].Setup(i);
        ledFaders[i].Set(ledFaders[i].maxBright,500*(i+1)); 
    }
    unsigned long prevMillis = millis();
    while(prevMillis+1000>millis()) {
        for(byte i=0;i<5;i++) ledFaders[i].Update();
    }
    for(byte i=0;i<5;i++) {
        ledFaders[i].Set(0,500*(i+1)); 
    }
    prevMillis = millis();
    while(prevMillis+3000>millis()) {
        for(byte i=0;i<5;i++) ledFaders[i].Update();
    }


//welcome message
    if(debugSerial) Serial.println(); Serial.println();
    if(debugSerial) Serial.println(F("Welcome to Biodata Sonification .. now with BLE and Wifi!"));

   

 if(!digitalRead(buttonPin)) {
     if (debugSerial) Serial.println("Button Held at Bootup - Reset!");
     ledFaders[4].Set(255, 1000);
     ledFaders[4].Update();
     while(ledFaders[4].isRunning) {
        ledFaders[4].Update();
     }
     //reset memory - chromatic scale, channel 1, wifi off, ble on, root C
      EEPROM.write(0, defScale); EEPROM.write(1, channel); EEPROM.write(2,0); EEPROM.write(3,1); EEPROM.write(5, 0);
      EEPROM.commit();
         bleMIDI = 1;
         wifiMIDI = 0;
         //channel = 1;  //declared at top
         applyScale(defScale);
         root = 0;

     ledFaders[4].Set(0, 0); //does this set immediately?

  }

//read from memory and load
  byte scaleIndex = EEPROM.read(0);
  byte midiChannel = EEPROM.read(1);
  byte wifiPower = EEPROM.read(2);
  byte blePower = EEPROM.read(3);
  byte keybyte = EEPROM.read(4);
  if(keybyte != 1) { //if not initialized first time - Scale,channel,wifi,bluetooth, key, root
    //init for millersville
    //EEPROM.write(0, 0); EEPROM.write(1, 1); EEPROM.write(2,1); EEPROM.write(3,0); EEPROM.write(4,1);
     //normal init - ble ON, wifi OFF, root C
      EEPROM.write(0, defScale); EEPROM.write(1, channel); EEPROM.write(2,0); EEPROM.write(3,1); EEPROM.write(4,1); EEPROM.write(5, 0);
      EEPROM.commit();
      if (debugSerial) Serial.println("EEPROM Initialized - First time! BLE ON, WiFi OFF");
         scaleIndex = EEPROM.read(0);
         midiChannel = EEPROM.read(1);
         wifiPower = EEPROM.read(2);
         blePower = EEPROM.read(3);
  }

  byte rootByte = EEPROM.read(5);
  if(rootByte > 11) rootByte = 0; // uninitialized EEPROM on older devices
  
  channel = midiChannel; //need two bytes to hold up to 16 channels!!
  root = rootByte;
  
  applyScale(scaleIndex);
  wifiMIDI = wifiPower;
  bleMIDI = blePower;
  

  if (serialMIDI)  setupSerialMIDI(); // MIDI hardware serial output
  
  // Sensores Ambientales: Inicializar ANTES de WiFi (como código funcional)
  if (wifiMIDI)    {
    setupEnvironmentalSensors(); // CRÍTICO: Inicializar sensores ANTES de WiFi
    setupWifi(); 
    // MQTT Buffer: Inicializar buffer MQTT si WiFi está activo
    setupMQTT();
    bufferEnabled = true;
  }
  else { WiFi.disconnect(true); delay(1);   WiFi.mode(WIFI_OFF); delay(1);} //turn wifi radio off
  if (bleMIDI)     bleSetup();

  //setup pulse input pin
  attachInterrupt(interruptPin, sample, RISING);  //begin sampling from interrupt

  for(byte i=0;i<5;i++) { ledFaders[i].Update(); ledFaders[i].Set(0,2000); } //all fade off

} //end setup(){}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void loop() {
  //manage time
  currentMillis = millis();
  if (wifiMIDI) MIDI.read();
  
  //analyze data when the buffer is full
  if (sampleIndex >= samplesize)  {
    analyzeSample();
  }

  // Manage MIDI
  checkNote();  //turn off expired notes
  checkControl();  //update control value

  // MQTT Buffer: solo con WiFi asociado (evita reconectar/publicar sin enlace)
  if(bufferEnabled) {
    if(WiFi.status() == WL_CONNECTED) {
      if(!mqtt.connected()) {
        reconnectMQTT();
      }
      if(mqtt.connected() || hasPendingMQTTData()) {
        mqtt.loop();
      }
      checkBufferTimer();
    }
    checkEnvironmentalTimer();
  }

  if (wifiMIDI) {
    wifiMulti.run();
  }

  // Mange LEDs
  for(byte i=0;i<5;i++) ledFaders[i].Update();
  
  //Manage pot and button
   checkKnob();  // updates threshold in main biodata mode
 

//this keeps the Red LED on when wifi is not connected
  if(wifiMIDI && WiFi.status() != WL_CONNECTED) { ledFaders[0].Set(ledFaders[0].maxBright,0); }

  // BLE connect: blue LED on for 1 s, then off
  if (bleConnectFlashPending) {
    bleConnectFlashPending = false;
    ledFaders[3].Set(ledFaders[3].maxBright, 0);
    bleConnectFlashOffAt = currentMillis + 1000;
  }
  if (bleConnectFlashOffAt != 0 && currentMillis >= bleConnectFlashOffAt) {
    ledFaders[3].Set(0, 0);
    bleConnectFlashOffAt = 0;
  } 

  checkButton();

} //end loop(){}
