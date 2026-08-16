# BiodataFeather
Feather daughter board for the Adafruit ESP32 and Huzzah (and others), allowing BLE and RTPMIDI via wifi.
Still a work in progress, and can provide the framework for anyone to use other Adafruit Feather boards
     Lots of Biodata information on my website electricityforprogress.com
See also the project submission on [Hackaday.io](https://hackaday.io/project/182849-biodata-sonification-v05)

3D Enclosure Model: https://www.tinkercad.com/things/8HZnh0ERNtr-biodata-wifi-snap-enclosure

Menu Modes:

From normal Biodata mode, press the button to enter menu selection and turn the knob to choose a menu (LED color). Click again to enter that menu, turn the knob to choose a value, then click to save and return to Biodata mode. Scale and root note are applied live while you turn the knob; the click stores them in EEPROM so they reload on boot.

Hard reset: hold the button while powering on. After the light show, factory defaults are applied (Pentatonic minor, Channel 1, Bluetooth ON, Wifi OFF, Root C).

LED bit order for binary values (LSB first): **Red = 1, Yellow = 2, Green = 4, Blue = 8, White = 16**.

Red - Note Scaling - 12 scales, shown in binary on the five LEDs (same scheme as MIDI channel). Indices 0-3 match the previous firmware mapping.

| LEDs (R Y G B W) | # | Scale |
|---|---|---|
| all off | 0 | Chromatic |
| R | 1 | Natural minor |
| Y | 2 | Major |
| R+Y | 3 | Pentatonic minor |
| G | 4 | Pentatonic major |
| R+G | 5 | Dorian |
| Y+G | 6 | Mixolydian |
| R+Y+G | 7 | Lydian |
| B | 8 | Phrygian |
| R+B | 9 | Harmonic minor |
| Y+B | 10 | Whole tone |
| R+Y+B | 11 | Hirajoshi |

Orange - MIDI Channel - 1-16, displayed in binary on the five LEDs (1 = Red, 2 = Yellow, 3 = Red+Yellow, …, 16 = White). Click to save.

Green - Wifi On/Off - default is off. Red flash = off, White flash = on. If firmware has WiFi credentials, this enables WiFi MIDI / MQTT.

Blue - Bluetooth On/Off - default is ON. Red flash = off, White flash = on.

White - Root note - tonic of the current scale, 0-11 (C, C#, D, …, B), displayed in binary like the scale menu. The White LED stays on as a menu marker. New notes use the chosen root immediately; click to save.
