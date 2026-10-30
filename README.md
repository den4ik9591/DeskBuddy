# DeskBuddy

A little desk robot with an animated screen face. It can turn, nod and tilt its head, blink, and change mood when you press the button on its base. It runs on a custom ESP32-S3 PCB I designed in KiCad, inside a 3D-printed case I designed in Fusion 360.

![DeskBuddy case](Week4-DeskBuddy/Images/Screenshot%202026-10-30%20121147.png)
![DeskBuddy inside the head](Week4-DeskBuddy/Images/Screenshot%202026-10-30%20121157.png)

## How it works

- **Brain:** Waveshare ESP32-S3-Zero on a custom PCB, powered over USB-C.
- **Face:** 1.3" 240x240 ST7789 IPS screen over SPI. The firmware draws eyes and a mouth, with blinking and four moods (happy, sleepy, curious, surprised).
- **Movement:** three MG90S servos. Pan turns the neck, nod moves the head up and down, and tilt rolls the face sideways.
- **Power:** the servos run from a separate 5V supply through a screw terminal, with a 1000 µF capacitor to smooth out current spikes.
- **Button:** cycles through the moods.

| Part | GPIO |
|---|---|
| Servos: pan / nod / tilt | 4 / 5 / 6 |
| Button (to GND) | 7 |
| Screen: SCL / SDA / RES / DC / BLK | 12 / 11 / 9 / 10 / 8 |

## The case

The case has seven printed parts: base, lid, button plunger, neck, head pod, face shell and face back. I checked it in Fusion 360 with the real servo and screen models for collisions at pan ±80°, nod ±25° and tilt ±20°. It is toleranced for FDM printing, with servo pockets 0.5 mm wider than the servo, M2 pilot holes of 1.7 mm and 2 mm walls.

I first tried getting AI to make the case, but it was bad, so I redesigned it myself, which took ages.

## Files

- `Week4-DeskBuddy/Schematics/` KiCad schematic, PCB and STEP
- `Week4-DeskBuddy/Final files/` Fusion 360, STL and 3MF files, plus the Arduino code in `DeskBuddy/DeskBuddy.ino`
- `BOM.csv` bill of materials

## Build it

1. Order the PCB and the parts in `BOM.csv`, and print the case.
2. Solder everything onto the PCB.
3. In the Arduino IDE, install Adafruit GFX, Adafruit ST7735 and ST7789, and ESP32Servo.
4. Select "ESP32S3 Dev Module", turn on USB CDC On Boot, and upload the sketch.
5. Connect 5V to the screw terminal and assemble the case.
