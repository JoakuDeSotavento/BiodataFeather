# How to Update Your Biodata Firmware

From time to time we publish improvements to the Biodata: new scales, smoother connections, small fixes, extra features. Those updates live in this GitHub repository:

**[github.com/JoakuDeSotavento/BiodataFeather](https://github.com/JoakuDeSotavento/BiodataFeather/)**

“Firmware” is simply the program that runs inside the Biodata. Updating it is like giving the device a new brain. You copy that program from your computer into the box with a USB cable.

You do **not** need to be a programmer. You do not need to understand the code. You only need to follow the steps in order.

**Please read this whole page before you press Upload.** Most problems happen when someone tries to send the code before Arduino knows the board, or before the extra libraries are installed.

---

## What you need

- A computer (Windows, Mac, or Linux)
- Your Biodata
- A USB cable that can carry **data**, not only charge. If the computer never “sees” the device, try another cable first
- About 20 quiet minutes the first time. Later updates are faster, because the tools stay on your computer

---

## The big picture

There are five stages. Do them in this order, and do not skip ahead.

1. Install the Arduino program (this is the app that talks to the Biodata).
2. Teach Arduino about the ESP32-S3 chip inside the Biodata.
3. **Install every library on the list below.** Do this *before* you open the code or try to compile.
4. Download the latest code from GitHub and add a small `secrets.h` file.
5. Choose the board settings, plug in the Biodata, and press Upload.

If a step fails, stop there. The “If something goes wrong” section at the end is for that.

---

## 1. Install Arduino IDE

Arduino IDE is a free app made for sending programs to small devices.

1. Go to [arduino.cc/en/software](https://www.arduino.cc/en/software).
2. Download **Arduino IDE 2** for your computer.
3. Install it like any other app, then open it once so it can finish setting up.

You will see a mostly empty window with a play button and a right-arrow button. The play button **verifies** (checks) the code. The right-arrow **uploads** it to the Biodata. We will not press those until the end.

---

## 2. Teach Arduino about the ESP32 board

The Biodata is built around an **Adafruit Feather ESP32-S3**. Arduino does not know that chip until you add a board package.

### Add the board address

1. In Arduino, open **File → Preferences** (on a Mac: **Arduino IDE → Settings**).
2. Find **Additional boards manager URLs**.
3. Paste this address:

```
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

If something is already there, add a comma, then paste the new address on the same line — or click the small icon on the right and add it on its own line.

4. Click **OK**.

### Install the ESP32 package

1. Open **Tools → Board → Boards Manager…**  
   (In Arduino IDE 2 you can also click the board icon on the left sidebar.)
2. Search for **esp32**.
3. Install **esp32** by **Espressif Systems**.
4. Choose version **3.3.2** if you can see a version list. That is the version this firmware is written for. If 3.3.2 is not listed, pick the newest 3.3.x.

This download can take a few minutes. Let it finish. Close and reopen Arduino if the new boards do not appear under **Tools → Board**.

You still have not uploaded anything. That is correct.

---

## 3. Install the libraries — do this before you compile

This is the most important section on the page.

The Biodata code uses extra pieces of software called **libraries**. They are like plugins. Arduino does not include them until you install them. If you try to Verify or Upload too early, you will get a wall of red errors that look scary and usually mean “a library is missing.”

Install **all** of the libraries below, even if you do not use Wi-Fi, MQTT, or the environmental sensors. The current firmware is one program; it needs the full set in order to compile.

### How to open the Library Manager

1. In Arduino, go to **Sketch → Include Library → Manage Libraries…**  
   (Or click the books icon on the left sidebar.)
2. A search box appears.
3. Type the **search name** from the table.
4. Check the **author** so you install the right one.
5. Click **Install**. If Arduino asks “Would you like to install also the dependencies?”, say **Yes**.

Do that for every row marked “Library Manager”.

### Libraries to install from Library Manager

| Search for | Author | Version notes |
|---|---|---|
| **PubSubClient** | Nick O'Leary | Latest is fine. This talks to MQTT. |
| **ArduinoJson** | Benoit Blanchon | Install **version 6**, not 7. Open the version menu next to Install and pick a 6.x release (for example 6.21.x). Version 7 will not compile this firmware. |
| **AppleMIDI** | lathoub | This is Wi-Fi MIDI (RTP-MIDI). |
| **MIDI Library** | Francois Best | AppleMIDI needs this. Search “MIDI Library” and pick the one by Francois Best (FortySevenEffects). |
| **Adafruit LTR329 and LTR303** | Adafruit | Light sensor. When asked about extra libraries, click **Install all**. |
| **Adafruit BusIO** | Adafruit | The light-sensor library needs this. Install it even if Arduino already offered it as a dependency. |

After each install, wait until it says it finished. Then search for the next one.

### One library that is not in Library Manager

The temperature / humidity / air sensor uses **DFRobot_BME68x**, which you add as a ZIP file.

1. Open [github.com/DFRobot/DFRobot_BME68x](https://github.com/DFRobot/DFRobot_BME68x).
2. Click the green **Code** button, then **Download ZIP**. Save the file somewhere easy, like Downloads.
3. In Arduino: **Sketch → Include Library → Add .ZIP Library…**
4. Choose the ZIP you just downloaded. You do not need to unzip it first.

Arduino will say the library was installed.

### Already included — do not install these yourself

These come with the ESP32 board package. You should **not** hunt for them in Library Manager:

- Wi-Fi, Bluetooth, EEPROM, Wire
- `USB` and `USBMIDI` (the USB MIDI support built into ESP32 3.3.x)

Please **do not** install **Adafruit TinyUSB** for this firmware. This Biodata uses the USB MIDI that already lives in the ESP32 package. Mixing both can make the upload fail in confusing ways.

### Quick checklist

Tick these off before you download the code:

- [ ] PubSubClient (Nick O'Leary)
- [ ] ArduinoJson **6.x** (Benoit Blanchon) — not 7
- [ ] AppleMIDI (lathoub)
- [ ] MIDI Library (Francois Best)
- [ ] Adafruit LTR329 and LTR303
- [ ] Adafruit BusIO
- [ ] DFRobot_BME68x (from the GitHub ZIP)

When that list is done, you are ready for the code.

---

## 4. Download the firmware from GitHub

1. Open **[github.com/JoakuDeSotavento/BiodataFeather](https://github.com/JoakuDeSotavento/BiodataFeather/)**.
2. Click the green **Code** button, then **Download ZIP**.
3. Unzip the file. You will get a folder named something like `BiodataFeather-master`.
4. Inside it, open **`00_Biotada_MolinoLab_versions`**.
5. Choose the firmware folder with the **highest number**. Right now that is:

**`Biodata_Feather_ESP32_11_Environmental`**

That folder is the sketch. Leave it as it is. Arduino expects all of those `.ino` files to stay together in the same folder, with the same name as the main file.

### Make the `secrets.h` file (required)

The code looks for a small file called `secrets.h` with Wi-Fi and MQTT names. GitHub does not include a real one, on purpose, so passwords stay private. **Without this file, the code will not compile.**

1. In that firmware folder, find **`secrets_sample.h.txt`**.
2. Copy it.
3. Rename the copy to **`secrets.h`** (remove the `.txt`).  
   The name must be exactly `secrets.h`.
4. Open `secrets.h` in Notepad (Windows), TextEdit (Mac), or Arduino itself.
5. Put your Wi-Fi name and password in `WIFI_SSID` and `WIFI_PASSWORD`. Leave the quotes around them.

If you only use Bluetooth or USB, you can leave the example names. The file still has to exist. Extra Wi-Fi lines (`WIFI_SSID_2`, and so on) can stay as they are.

Do not publish `secrets.h` anywhere. It is meant to stay on your computer.

---

## 5. Open the sketch and choose the board

1. In Arduino: **File → Open…**
2. Go to the firmware folder and open **`Biodata_Feather_ESP32_11_Environmental.ino`**.  
   Several tabs will appear at the top (`Main`, `MIDI`, `Environmental`…). That is normal. They are all one program.
3. Go to **Tools → Board → esp32** and pick the Feather that matches your hardware:
   - **Adafruit Feather ESP32-S3 No PSRAM** — the usual Biodata board
   - **Adafruit Feather ESP32-S3 2MB PSRAM** — only if your Feather is the PSRAM version
4. Under **Tools**, set these USB options (names can vary slightly by version):

| Setting | Choose |
|---|---|
| **USB CDC On Boot** | Enabled |
| **USB Mode** | USB-OTG (TinyUSB) |
| **USB DFU On Boot** | Disabled |
| **USB Firmware MSC On Boot** | Disabled |
| **Upload Speed** | 921600 (try 115200 if upload keeps failing) |

Those USB settings matter. If USB Mode is wrong, the Biodata may compile and upload, then never appear as a MIDI device.

---

## 6. Plug in and upload

1. Connect the Biodata to the computer with the USB cable.
2. Go to **Tools → Port** and choose the new port.  
   On Windows it looks like **COM3** or **COM7**. On a Mac it looks like **usbmodem** or **usbserial**.
3. Click the **Verify** button (the check mark / play icon) once.  
   The first check can take a couple of minutes. Wait until the bottom of the window says it is done.
4. If Verify succeeds, click **Upload** (the right arrow).

The lights on the Biodata may flicker. When Arduino says **Done uploading**, unplug and plug it back in, or tap the reset button on the Feather. Then use the Biodata as usual.

After a firmware update, if Bluetooth or the menu feels odd, do a restore: **hold the Biodata button while you switch it on**, wait for the light show, and you are back to the factory defaults (pentatonic minor, channel 1, Bluetooth on, Wi-Fi off, root C).

---

## If something goes wrong

### Red errors about a missing `.h` file

Arduino is telling you a library (or `secrets.h`) is missing.

- `secrets.h: No such file` → you skipped the copy/rename step in section 4.
- `PubSubClient.h`, `ArduinoJson.h`, `AppleMIDI.h`, `MIDI.h`, `Adafruit_LTR329_LTR303.h`, `DFRobot_BME68x.h` → that library is not installed. Go back to section 3. Close and reopen Arduino after installing.
- ArduinoJson errors that mention `StaticJsonDocument` → you installed version **7**. Uninstall it and install **6.x**.

### No port in Tools → Port

- Try another USB cable (charge-only cables are very common).
- Try another USB socket on the computer.
- Put the Feather into bootloader mode: hold the small **BOOT** button on the Feather board (this is not the big Biodata menu button), tap **RESET**, then release BOOT. Look at **Tools → Port** again.

### Upload starts, then fails

- Close any app that might be holding the USB port (Serial Monitor, a DAW, Perfect Bluetooth MIDI, a browser tab using Web MIDI).
- Lower **Upload Speed** to 115200.
- Use bootloader mode (BOOT + RESET) and upload again.
- On Windows, wait a few seconds after plugging in before you click Upload.

### It uploads, but the computer does not see USB MIDI

Check **USB Mode = USB-OTG (TinyUSB)** and **USB CDC On Boot = Enabled**, then upload again. After it finishes, unplug the cable and plug it back in so the computer rediscovers the device.

### I am afraid I will “break” the Biodata

You will not brick it by following this guide. If an upload fails, the old program is often still there. If the new program misbehaves, hold the menu button at startup to restore settings, or simply upload again.

---

## Next time you update

You already did the hard part. For a later release:

1. Download a fresh ZIP from [the GitHub repository](https://github.com/JoakuDeSotavento/BiodataFeather/).
2. Open the newest folder inside `00_Biotada_MolinoLab_versions`.
3. Copy your existing `secrets.h` into that new folder (or make it again from the sample).
4. Confirm the libraries are still installed.
5. Open the `.ino`, pick the same board and USB settings, and press Upload.

New improvements land in that same repository. If a future firmware ever needs an extra library, it will be listed with the release — but start from this page, and install libraries **before** you compile.
