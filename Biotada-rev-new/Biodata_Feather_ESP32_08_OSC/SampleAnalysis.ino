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
    microseconds = samples[sampleIndex] + microseconds;  //rebuild micros() value w/o recalling
    //micros() is very slow
    //try a higher precision counter
    //samples[index] = ((timer0_overflow_count << 8) + TCNT0) - microseconds;
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

  if (sampleIndex >= samplesize) {          //array is full
    unsigned long sampanalysis[analysize];  //copy to new array - is this needed?
    int setnote = 0;
    for (byte i = 0; i < analysize; i++) {
      //skip first element in the array
      sampanalysis[i] = samples[i + 1];  //load analysis table (due to volitle)
      //manual calculation
      if (sampanalysis[i] > maxim) { maxim = sampanalysis[i]; }
      if (sampanalysis[i] < minim) { minim = sampanalysis[i]; }
      averg += sampanalysis[i];
      stdevi += sampanalysis[i] * sampanalysis[i];  //prep stdevi
    }

    //calculation
    averg = averg / analysize;
    stdevi = sqrt(stdevi / analysize - averg * averg);  //calculate stdevu
    if (stdevi < 1) { stdevi = 1.0; }                   //min stdevi of 1
    delta = maxim - minim;

    //**********perform change detection
    if (delta > (stdevi * threshold)) {
      change = 1;
    }
    //*********

    if (change) {

      //analyze the values
      dur = 150 + (map(delta % 127, 0, 127, 100, 5500));  //length of note
      ccValue = delta % 127;
      ramp = 3 + (dur % 100);  //control slide rate, min 25 (or 3 ;)
      vel = 90;                // this value should modulate
      //  * Velocity - five levels of control: red-100,yellow-accent(90/120),
      //     green-(75,95,120),blue-musical map(value,x,y,50,120),
      //     white-fluent map(value,x,y,0,127)
      vel = delta % 127;
      //if(velMode == 0) vel = 90;
      //if(velMode == 1) if(vel < 120) vel = 90;
      //if(velMode == 2) if(vel < 120 && vel > 80) vel = 90; else if(vel>=80) vel =65;
      //if(velMode == 3) vel = map(vel,0,127,50,120);
      //if(velMode == 5) vel = delta%127;
      vel = map(vel, 0, 127, 80, 110);  //musical velocity range

      //set scaling, root key, note
      setnote = map(averg % 127, 0, 127, noteMin, noteMax);  //derive note, min and max note
      setnote = scaleNote(setnote, scaleSelect, root);       //scale the note
      // setnote = setnote + root; // (apply root?)
      setNote(setnote, vel, dur, channel);  //modify velocity, using note repetition or something?

      //derive control parameters and set
      setControl(controlNumber, controlMessage.value, ccValue, ramp);  //set the ramp rate for the control
    }

    //rawSerial data output
    //write a value ever rawSerialDelay milliseconds to slow down the data flow
    //    also write if a change is detected at any time!
    if (rawSerial && (change || (currentMillis > rawSerialTime + rawSerialDelay))) {
      rawSerialTime = currentMillis;  //reset timer

      int _averg = map(averg,0,600,0,100);
      float _stdevi = stdevi;
      int _threshold = map(constrain(threshold*stdevi,0,300),0,300,0,100);
      int _delta = map(delta,0,300,0,100);
      int _change = change * 90;
      //write all primary values, need to map range for visualization...
      Serial.print(_averg); 
      Serial.print(","); //Average
      Serial.print(_stdevi); 
      Serial.print(","); //standard deviation
      Serial.print(_threshold);
      Serial.print(",");  //threshold compare against delta
      Serial.print(_delta);
      Serial.print(",");          //delta
      Serial.print(_change);  //Serial.print(","); //change detected
      //change and Note/CC
      /*
      if(change) { // duration, velocity, note, CC
        Serial.print(change*100); Serial.print(","); //change detected
        Serial.print(setnote); Serial.print(","); //Note
        Serial.print(vel); Serial.print(","); //Velocity
        Serial.print(map(dur,100,5500,1,100)); Serial.print(","); //Duration //need to map this to values
        Serial.print(ccValue);// Serial.print(","); //CC
      }
      else { //pad with 0's although this might be ugly...
        Serial.print(0); Serial.print(","); //Change
        Serial.print(0); Serial.print(","); //Note
        Serial.print(0); Serial.print(","); //Velocity
        Serial.print(0); Serial.print(","); //Duration
        Serial.print(0); //Serial.print(","); //CC
      }
     */
      Serial.println();  //end of raw data packet
    }

    ///////////////raw data output by OSC //////////////////////////
    //write a value ever rawSerialDelay milliseconds to slow down the data flow
    //    also write if a change is detected at any time!
    if (rawOSC && (change || (currentMillis > rawSerialTime + rawSerialDelay))) {
      rawSerialTime = currentMillis;  //reset timer

      int _averg = map(averg,0,600,0,100);
      float _stdevi = stdevi;
      int _threshold = map(constrain(threshold*stdevi,0,300),0,300,0,100);
      int _delta = map(delta,0,300,0,100);
      int _change = change * 90;

      // Function to send OSC
      sendOSCMessage(_averg, _stdevi, _threshold, _delta, _change);

    }

    ///////////////raw data output to a firebase database //////////////////////////
    //write a value ever rawSerialDelay milliseconds to slow down the data flow
    //    also write if a change is detected at any time!
    if (rawFB && (change || (currentMillis > rawSerialTime + rawSerialDelay))) {
      rawSerialTime = currentMillis;  //reset timer

      int _averg = map(averg,0,600,0,100);
      float _stdevi = stdevi;
      int _threshold = map(constrain(threshold*stdevi,0,300),0,300,0,100);
      int _delta = map(delta,0,300,0,100);
      int _change = change * 90;

      if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 15000 || sendDataPrevMillis == 0)){
        sendDataPrevMillis = millis();

        // Write average (int) on the database path biodataTest/average
        if (Firebase.RTDB.setInt(&fbdo, "biodataTest/average", _averg)){
          Serial.println("PASSED");
          Serial.println("PATH: " + fbdo.dataPath());
          Serial.println("TYPE: " + fbdo.dataType());
        }
        else {
          Serial.println("FAILED");
          Serial.println("REASON: " + fbdo.errorReason());
        }
        
        // Write standard deviation (float) on the database path biodataTest/stdevi
        if (Firebase.RTDB.setFloat(&fbdo, "biodataTest/stdevi", _stdevi)){
          Serial.println("PASSED");
          Serial.println("PATH: " + fbdo.dataPath());
          Serial.println("TYPE: " + fbdo.dataType());
        }
        else {
          Serial.println("FAILED");
          Serial.println("REASON: " + fbdo.errorReason());
        }

        // Write threshold (int) on the database path biodataTest/threshold
        if (Firebase.RTDB.setInt(&fbdo, "biodataTest/threshold", _threshold)){
          Serial.println("PASSED");
          Serial.println("PATH: " + fbdo.dataPath());
          Serial.println("TYPE: " + fbdo.dataType());
        }
        else {
          Serial.println("FAILED");
          Serial.println("REASON: " + fbdo.errorReason());
        }

        // Write delta (int) on the database path biodataTest/delta
        if (Firebase.RTDB.setInt(&fbdo, "biodataTest/delta", _delta)){
          Serial.println("PASSED");
          Serial.println("PATH: " + fbdo.dataPath());
          Serial.println("TYPE: " + fbdo.dataType());
        }
        else {
          Serial.println("FAILED");
          Serial.println("REASON: " + fbdo.errorReason());
        }

        // Write change (int) on the database path biodataTest/change
        if (Firebase.RTDB.setInt(&fbdo, "biodataTest/change", _change)){
          Serial.println("PASSED");
          Serial.println("PATH: " + fbdo.dataPath());
          Serial.println("TYPE: " + fbdo.dataType());
        }
        else {
          Serial.println("FAILED");
          Serial.println("REASON: " + fbdo.errorReason());
        }

      }

    }

    //reset array for next sample
    sampleIndex = 0;
  }
}
