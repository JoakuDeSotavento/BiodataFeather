//*******Analysis of sample data
/*
 * Fill an array with samples
 * calcluate max, min, delta, average, standard deviation
 * compare against a 'change threshold' delta > (stdevi * threshold)
 * when change is detected map note pitch, velocity, duration
 * set the sample array size, the effective 'resolution' of change detection
 * using micros() for sample values, which is irratic, use a better method
 * sampling at the 'rising' edge, which counts a full period
 */
//*******

void sample() {
  if (sampleIndex < samplesize) {
    samples[sampleIndex] = micros() - microseconds;
    microseconds = samples[sampleIndex] + microseconds;
    sampleIndex += 1;
  }
}

void analyzeSample() {
  unsigned long averg = 0;
  unsigned long maxim = 0;
  unsigned long minim = 100000;
  float stdevi = 0;
  unsigned long delta = 0;
  byte change = 0;
  int dur = 0;
  byte ccValue = 0;
  int ramp = 0;
  byte vel = 0;

  if (sampleIndex >= samplesize) {
    unsigned long sampanalysis[analysize];
    int setnote = 0;
    
    for (byte i = 0; i < analysize; i++) {
      sampanalysis[i] = samples[i + 1];
      if (sampanalysis[i] > maxim) { maxim = sampanalysis[i]; }
      if (sampanalysis[i] < minim) { minim = sampanalysis[i]; }
      averg += sampanalysis[i];
      stdevi += sampanalysis[i] * sampanalysis[i];
    }

    // Cálculos
    averg = averg / analysize;
    stdevi = sqrt(stdevi / analysize - averg * averg);
    if (stdevi < 1) { stdevi = 1.0; }
    delta = maxim - minim;

    // Detección de cambios
    if (delta > (stdevi * threshold)) {
      change = 1;
    }

    if (change) {
      // Analizar valores
      dur = 150 + (map(delta % 127, 0, 127, 100, 5500));
      ccValue = delta % 127;
      ramp = 3 + (dur % 100);
      vel = map(delta % 127, 0, 127, 80, 110);

      // Configurar nota
      setnote = map(averg % 127, 0, 127, noteMin, noteMax);
      setnote = scaleNote(setnote, scaleSelect, root);
      setNote(setnote, vel, dur, channel);

      // Configurar control
      setControl(controlNumber, controlMessage.value, ccValue, ramp);
    }

    // Salida de datos seriales
    if (rawSerial && (change || (currentMillis > rawSerialTime + rawSerialDelay))) {
      rawSerialTime = currentMillis;

      int _averg = map(averg, 0, 600, 0, 100);
      float _stdevi = stdevi;
      int _threshold = map(constrain(threshold * stdevi, 0, 300), 0, 300, 0, 100);
      int _delta = map(delta, 0, 300, 0, 100);
      int _change = change * 90;

      if(debugSerial) {
        Serial.printf("BIO: %d,%.2f,%d,%d,%d\n", _averg, _stdevi, _threshold, _delta, _change);
      }
    }
  }
}

// ---------- Funciones MIDI ----------
void setNote(int value, int velocity, long duration, int notechannel) {
  // Buscar nota disponible en array
  for(int i = 0; i < polyphony; i++) {
    if(!noteArray[i].velocity) {
      noteArray[i].type = 0;
      noteArray[i].value = value;
      noteArray[i].velocity = velocity;
      noteArray[i].duration = currentMillis + duration;
      noteArray[i].channel = notechannel;
      
      // Enviar nota MIDI
      if(serialMIDI) midiSerial(144, channel, value, velocity);
      // if(usbmidi) usbMIDI.sendNoteOn(value, velocity, channel); // Comentado temporalmente
      // if(wifiMIDI && isConnected) MIDI.sendNoteOn(value, velocity, channel); // Comentado temporalmente
      
      // Indicar con LED
      setLED(i, ledBrightness[i]);
      
      break;
    }
  }
}

void setControl(int type, int value, int velocity, long duration) {
  controlMessage.type = type;
  controlMessage.value = value;
  controlMessage.velocity = velocity;
  controlMessage.period = duration;
  controlMessage.duration = currentMillis + duration;
}

void checkControl() {
  signed int distance = controlMessage.velocity - controlMessage.value;
  
  if(distance != 0) {
    if(currentMillis > controlMessage.duration) {
      controlMessage.duration = currentMillis + controlMessage.period;
      
      if(distance > 0) { 
        controlMessage.value += 1; 
      } else { 
        controlMessage.value -= 1; 
      }
      
      // Enviar mensaje de control MIDI
      if(serialMIDI) midiSerial(176, channel, controlMessage.type, controlMessage.value);
      // if(usbmidi) usbMIDI.sendControlChange(controlNumber, controlMessage.value, channel); // Comentado temporalmente
      // if(wifiMIDI && isConnected) MIDI.sendControlChange(controlNumber, controlMessage.value, channel); // Comentado temporalmente
    }
  }
}

void checkNote() {
  for (int i = 0; i < polyphony; i++) {
    if(noteArray[i].velocity) {
      if (noteArray[i].duration <= currentMillis) {
        // Enviar NoteOff
        if(serialMIDI) midiSerial(144, channel, noteArray[i].value, 0);
        // if(usbmidi) usbMIDI.sendNoteOn(noteArray[i].value, 0, channel); // Comentado temporalmente
        // if(wifiMIDI && isConnected) MIDI.sendNoteOff(noteArray[i].value, 0, channel); // Comentado temporalmente
        
        noteArray[i].velocity = 0;
        setLED(i, 0); // Apagar LED
      }
    }
  }
}

// ---------- Funciones de escala ----------
int scaleNote(int note, int* scale, int root) {
  int scaleLength = scale[0];
  int octave = (note - noteMin) / 12;
  int noteInOctave = (note - noteMin) % 12;
  
  // Buscar nota más cercana en la escala
  int closestNote = scale[1];
  int minDistance = abs(noteInOctave - scale[1]);
  
  for(int i = 2; i <= scaleLength; i++) {
    int distance = abs(noteInOctave - scale[i]);
    if(distance < minDistance) {
      minDistance = distance;
      closestNote = scale[i];
    }
  }
  
  return noteMin + (octave * 12) + closestNote + root;
}

// ---------- Función MIDI Serial (placeholder) ----------
void midiSerial(byte status, byte channel, byte data1, byte data2) {
  // Implementar si necesitas salida MIDI serial
  if(debugSerial) {
    Serial.printf("MIDI: %02X %02X %02X\n", status | (channel-1), data1, data2);
  }
}
