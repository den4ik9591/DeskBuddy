// DeskBuddy firmware - ESP32-S3-Zero + 1.3" ST7789 (7-pin, no CS) + 3x MG90S + button
// Libraries (Library Manager): Adafruit GFX Library, Adafruit ST7735 and ST7789 Library, ESP32Servo
// Board: "ESP32S3 Dev Module", USB CDC On Boot: Enabled
//
// Pins taken from the DeskBuddy PCB netlist:
//   Servo pan GP4, servo nod GP5, servo tilt GP6, button GP7 (to GND)
//   Screen: SCL GP12, SDA GP11, RES GP9, DC GP10, BLK GP8
//   (screen pin order assumed GND, VCC, SCL, SDA, RES, DC, BLK - check against your module's silkscreen)

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <ESP32Servo.h>

#define PIN_PAN   4
#define PIN_NOD   5
#define PIN_TILT  6
#define PIN_BTN   7
#define TFT_SCL  12
#define TFT_SDA  11
#define TFT_RES   9
#define TFT_DC   10
#define TFT_BLK   8

Adafruit_ST7789 tft = Adafruit_ST7789(&SPI, -1, TFT_DC, TFT_RES);  // no CS pin
GFXcanvas16 canvas(240, 240);  // draw off-screen, then push = no flicker (115 KB, fine on S3)

Servo sPan, sNod, sTilt;

// ---- servo limits (degrees, 90 = centre). Keep inside what the case allows. ----
const int PAN_MIN = 10,  PAN_MAX = 170;   // case tested to +/-80
const int NOD_MIN = 65,  NOD_MAX = 115;   // case tested to +/-25
const int TILT_MIN = 70, TILT_MAX = 110;  // case tested to +/-20
// flip any of these to -1 if an axis moves the wrong way
const int PAN_DIR = 1, NOD_DIR = 1, TILT_DIR = 1;

float curPan = 90, curNod = 90, curTilt = 90;
float tgtPan = 90, tgtNod = 90, tgtTilt = 90;

enum Mood { HAPPY, SLEEPY, CURIOUS, SURPRISED, MOOD_COUNT };
Mood mood = HAPPY;

void setTarget(float pan, float nod, float tilt) {
  tgtPan  = constrain(90 + PAN_DIR  * (pan  - 90), PAN_MIN, PAN_MAX);
  tgtNod  = constrain(90 + NOD_DIR  * (nod  - 90), NOD_MIN, NOD_MAX);
  tgtTilt = constrain(90 + TILT_DIR * (tilt - 90), TILT_MIN, TILT_MAX);
}

// smooth movement: limited step per update keeps current draw low (avoids brown-outs)
void updateServos() {
  static uint32_t last = 0;
  if (millis() - last < 20) return;
  last = millis();
  float maxStep = (mood == SURPRISED) ? 3.0 : 1.2;
  auto approach = [&](float &c, float t) {
    float d = t - c;
    if (fabs(d) <= maxStep) c = t; else c += (d > 0 ? maxStep : -maxStep);
  };
  approach(curPan, tgtPan);
  approach(curNod, tgtNod);
  approach(curTilt, tgtTilt);
  sPan.write((int)curPan);
  sNod.write((int)curNod);
  sTilt.write((int)curTilt);
}

// ---- face ----
float blink = 0;           // 0 open .. 1 closed
uint32_t nextBlink = 0;
int eyeLookX = 0, eyeLookY = 0;

void drawEye(int cx, int cy, int w, int h, float lid) {
  int hh = max(4, (int)(h * (1.0f - lid)));
  canvas.fillRoundRect(cx - w / 2, cy - hh / 2, w, hh, min(w, hh) / 2, ST77XX_WHITE);
  if (hh > 24) {  // pupil
    canvas.fillCircle(cx + eyeLookX, cy + eyeLookY, 16, ST77XX_BLACK);
    canvas.fillCircle(cx + eyeLookX + 6, cy + eyeLookY - 6, 5, ST77XX_WHITE);
  }
}

void drawFace() {
  canvas.fillScreen(ST77XX_BLACK);
  int w = 70, h = 90, ey = 100, lx = 70, rx = 170;
  float lid = blink;
  switch (mood) {
    case SLEEPY:    lid = max(lid, 0.55f); break;
    case SURPRISED: w = 80; h = 110; break;
    case CURIOUS:   break;
    default: break;
  }
  drawEye(lx, ey, w, h, lid);
  drawEye(rx, (mood == CURIOUS) ? ey - 12 : ey, w, (mood == CURIOUS) ? h + 14 : h, lid);

  // mouth
  switch (mood) {
    case HAPPY:
      canvas.fillCircle(120, 175, 36, ST77XX_WHITE);
      canvas.fillCircle(120, 160, 40, ST77XX_BLACK);
      break;
    case SLEEPY:
      canvas.fillRoundRect(105, 190, 30, 6, 3, ST77XX_WHITE);
      break;
    case CURIOUS:
      canvas.fillCircle(130, 190, 8, ST77XX_WHITE);
      break;
    case SURPRISED:
      canvas.fillCircle(120, 195, 14, ST77XX_WHITE);
      canvas.fillCircle(120, 195, 8, ST77XX_BLACK);
      break;
    default: break;
  }
  tft.drawRGBBitmap(0, 0, canvas.getBuffer(), 240, 240);
}

void updateBlink() {
  static uint32_t t = 0;
  if (millis() > nextBlink && blink == 0) { blink = 0.01; t = millis(); }
  if (blink > 0) {
    float p = (millis() - t) / 220.0f;               // 220 ms blink
    blink = (p < 0.5f) ? p * 2 : (p < 1.0f ? (1 - p) * 2 : 0);
    if (p >= 1.0f) { blink = 0; nextBlink = millis() + random(1500, 5000); }
  }
}

// ---- behaviour: pick something to look at every so often ----
uint32_t nextLook = 0;
void updateBehaviour() {
  if (millis() < nextLook) return;
  switch (mood) {
    case HAPPY:
      setTarget(random(55, 125), random(80, 100), random(80, 100));
      nextLook = millis() + random(1500, 4000);
      break;
    case SLEEPY:
      setTarget(90, NOD_MAX, 90);                    // head droops
      nextLook = millis() + 4000;
      break;
    case CURIOUS:
      setTarget(random(30, 150), 80, (random(2) ? 75 : 105));  // head tilt
      nextLook = millis() + random(1500, 3000);
      break;
    case SURPRISED:
      setTarget(90, NOD_MIN + 5, 90);
      nextLook = millis() + 1500;
      break;
    default: break;
  }
  eyeLookX = map((long)(tgtPan - 90), -80, 80, -14, 14);
  eyeLookY = map((long)(tgtNod - 90), -25, 25, -10, 10);
}

// ---- button ----
void updateButton() {
  static bool last = HIGH;
  static uint32_t lastChange = 0;
  bool now = digitalRead(PIN_BTN);
  if (now != last && millis() - lastChange > 40) {
    lastChange = millis();
    last = now;
    if (now == LOW) {
      mood = (Mood)((mood + 1) % MOOD_COUNT);
      nextLook = 0;
      Serial.printf("Mood: %d\n", mood);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_BTN, INPUT_PULLUP);
  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH);

  SPI.begin(TFT_SCL, -1, TFT_SDA, -1);
  tft.init(240, 240, SPI_MODE3);   // SPI_MODE3 is needed for CS-less ST7789 modules
  tft.setRotation(0);              // change 0-3 if the face is rotated in the case
  tft.fillScreen(ST77XX_BLACK);

  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  sPan.setPeriodHertz(50);  sNod.setPeriodHertz(50);  sTilt.setPeriodHertz(50);
  sPan.attach(PIN_PAN, 500, 2400);
  sNod.attach(PIN_NOD, 500, 2400);
  sTilt.attach(PIN_TILT, 500, 2400);
  sPan.write(90); sNod.write(90); sTilt.write(90);

  randomSeed(esp_random());
  nextBlink = millis() + 2000;
}

void loop() {
  updateButton();
  updateBehaviour();
  updateServos();
  updateBlink();
  static uint32_t lastDraw = 0;
  if (millis() - lastDraw > 33) { lastDraw = millis(); drawFace(); }  // ~30 fps
}
