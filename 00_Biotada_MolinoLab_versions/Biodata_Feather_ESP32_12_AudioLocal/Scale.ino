


int scaleSearch(int note, int scale[], int scalesize) {
  // include last degree (loop was i < scalesize and skipped it)
  for (byte i = 1; i <= scalesize; i++) {
    if (note == scale[i]) { return note; }
    if (note < scale[i]) { return scale[i]; }  // quantize up to next degree
  }
  // above the last degree: wrap to the tonic of the next octave
  return scale[1] + 12;
}


int scaleNote(int note, int scale[], int root) {
  //input note mod 12 for scaling, note/12 octave
  //search array for nearest note, return scaled*octave
  int scaled = note%12;
  int octave = note/12;
  int scalesize = (scale[0]);
  //search entire array and return closest scaled note
  scaled = scaleSearch(scaled, scale, scalesize);
  scaled = (scaled + (12 * octave)) + root; //apply octave and root

  return scaled;
}
