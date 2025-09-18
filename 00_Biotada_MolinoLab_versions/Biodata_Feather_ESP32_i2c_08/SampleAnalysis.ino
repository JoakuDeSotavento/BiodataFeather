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


void sample(){
  if(sampleIndex < samplesize) {
    samples[sampleIndex] = micros() - microseconds;
    microseconds = samples[sampleIndex] + microseconds; //rebuild micros() value w/o recalling
    //micros() is very slow
    //try a higher precision counter
    //samples[index] = ((timer0_overflow_count << 8) + TCNT0) - microseconds;
    sampleIndex += 1;

  } 
}

void analyzeSample()
{
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

  if (sampleIndex >= samplesize) { //array is full
    unsigned long sampanalysis[analysize]; //copy to new array - is this needed?
    int setnote=0;
    for (byte i=0; i<analysize; i++){ 
      //skip first element in the array
      sampanalysis[i] = samples[i+1];  //load analysis table (due to volitle)
      //manual calculation
      if(sampanalysis[i] > maxim) { maxim = sampanalysis[i]; }
      if(sampanalysis[i] < minim) { minim = sampanalysis[i]; }
      averg += sampanalysis[i];
      stdevi += sampanalysis[i] * sampanalysis[i];  //prep stdevi
    }

    //calculation
    averg = averg/analysize;
    stdevi = sqrt(stdevi / analysize - averg * averg); //calculate stdevu
    if (stdevi < 1) { stdevi = 1.0; } //min stdevi of 1
    delta = maxim - minim; 
    
    //**********perform change detection 
    if (delta > (stdevi * threshold)){
      change = 1;
    }
    //*********

    //raw data serial output with all parameters
    //dump analysis array
    //list Max, Min, delta, average, standard Deviation, threshold, Change
    if(rawSerial) {
     // for (byte i=0; i<analysize; i++) { Serial.print(sampanalysis[i]); Serial.print(","); }
      Serial.print(maxim); Serial.print(",");
      Serial.print(minim); Serial.print(",");
      Serial.print(delta); Serial.print(",");
      Serial.print(averg); Serial.print(",");
      Serial.print(stdevi); Serial.print(",");
      Serial.print(threshold); Serial.print(",");
      Serial.println(change);
    }
    
    
    if(change){       

      //analyze the values
       dur = 150+(map(delta%127,0,127,100,5500)); //length of note
       ccValue = delta%127;
       ramp = 3 + (dur%100) ; //control slide rate, min 25 (or 3 ;)
       vel = 90;  // this value should modulate 
        //  * Velocity - five levels of control: red-100,yellow-accent(90/120),
        //     green-(75,95,120),blue-musical map(value,x,y,50,120),
        //     white-fluent map(value,x,y,0,127)
        vel = delta%127;
        //if(velMode == 0) vel = 90;
        //if(velMode == 1) if(vel < 120) vel = 90;
        //if(velMode == 2) if(vel < 120 && vel > 80) vel = 90; else if(vel>=80) vel =65; 
        //if(velMode == 3) vel = map(vel,0,127,50,120);
        //if(velMode == 5) vel = delta%127;
        vel = map(vel,0,127,80,110); //musical velocity range
       
       //set scaling, root key, note
       setnote = map(averg%127,0,127,noteMin,noteMax);  //derive note, min and max note
       setnote = scaleNote(setnote, scaleSelect, root);  //scale the note
       // setnote = setnote + root; // (apply root?)
       setNote(setnote, vel, dur, channel); //modify velocity, using note repetition or something?
  
       //derive control parameters and set    
       setControl(controlNumber, controlMessage.value, ccValue, ramp); //set the ramp rate for the control

      
    } 

       
    //reset array for next sample
    sampleIndex = 0;
  }
}
