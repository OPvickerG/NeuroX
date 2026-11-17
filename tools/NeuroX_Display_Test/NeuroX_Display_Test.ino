/*
  NeuroX - display + touch test for Arduino UNO
  ---------------------------------------------
  Screen: 2.8" SPI TFT 240x320, ILI9341 driver, XPT2046 resistive touch
          (the red 14-pin module, the same screen as in the NeuroX box)

  Libraries (Arduino IDE > Tools > Manage Libraries):
    - "Adafruit ILI9341"      (say YES to install Adafruit GFX + BusIO too)
    - "XPT2046_Touchscreen"   by Paul Stoffregen

  Serial Monitor: 115200 baud. It tells you what each test checks.

  WIRING (UNO is 5 V, the screen's pins are 3.3 V -> use the dividers, see below)
    screen pin   ->  UNO
    VCC          ->  5V      (the module has its own 3.3 V regulator; leave jumper J1 OPEN)
    GND          ->  GND
    CS           ->  D10  *
    RESET        ->  D8   *
    DC           ->  D9   *
    SDI (MOSI)   ->  D11  *
    SCK          ->  D13  *
    LED          ->  3.3V   (backlight)
    SDO (MISO)   ->  D12    (direct, no divider)
    T_CLK        ->  D13  * (same wire as SCK, after the divider)
    T_CS         ->  D4   *
    T_DIN        ->  D11  * (same wire as SDI, after the divider)
    T_DO         ->  D12    (same wire as SDO, direct)
    T_IRQ        ->  D3     (direct; not wired? set TOUCH_IRQ to 255)
    SD_xxx pins  ->  not used

  * = through a voltage divider: UNO pin -> 1k -> screen pin, and screen pin -> 2k -> GND
      (2k = two 1k in series, or use 2.2k). 6 dividers: D13, D11, D10, D9, D8, D4.
      A CD4050 / 74HC4050 chip does the same job.
      Direct 5 V often "works" but can damage the screen over time or give a white screen.

  TESTS
    1. talks to the display and prints its ID registers (Serial)
    2. full-screen colours, with how long each fill took
    3. test picture: 1-pixel border, corner marks, colour bars, grey ramp, text
    4. all 4 rotations
    5. touch: 3-point calibration + a check point in the middle
    6. paint mode: draw with your finger / a stylus, colour palette, CLEAR
       - hold CLEAR for 3 s, or send 'c' in the Serial Monitor = calibrate again
*/
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <XPT2046_Touchscreen.h>

// Your board says "2.8 TFT 240xRGBx320 V1.1" - normally an ILI9341 chip.
// Screen stays WHITE although the wiring is checked? Some batches use an ST7789 chip:
// set this to 1, install "Adafruit ST7735 and ST7789 Library", upload again.
#define USE_ST7789 0
#define SPI_HZ  0          // 0 = full speed. Garbled picture? try 4000000 or 1000000

#if USE_ST7789
  #include <Adafruit_ST7789.h>
#else
  #include <Adafruit_ILI9341.h>
#endif

// ---------------- pins ----------------
#define TFT_CS     10
#define TFT_DC      9
#define TFT_RST     8
#define TOUCH_CS    4
#define TOUCH_IRQ   3      // 255 if T_IRQ is not connected

#define Z_MIN     300      // touch pressure needed to count as a press

#if USE_ST7789
  Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
#else
  Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
#endif

// colours (RGB565, same values for both chips)
#define C_BLACK     0x0000
#define C_NAVY      0x000F
#define C_DARKGREY  0x7BEF
#define C_LIGHTGREY 0xC618
#define C_BLUE      0x001F
#define C_GREEN     0x07E0
#define C_CYAN      0x07FF
#define C_RED       0xF800
#define C_MAGENTA   0xF81F
#define C_YELLOW    0xFFE0
#define C_WHITE     0xFFFF
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

const int W = 320, H = 240;            // landscape (rotation 1)
const int BAR = 28;                    // toolbar height (paint mode)
const int STAT = 12;                   // status line height (paint mode)

// touch calibration: screen = a * raw + b   (calSwap = raw x/y are swapped)
bool  calSwap = false;
float ax = 0, bx = 0, ay = 0, by = 0;

// paint mode
const uint16_t PAL[6] = { C_WHITE, C_RED, C_GREEN, C_BLUE, C_YELLOW, C_CYAN };
uint16_t ink = C_WHITE;
bool down = false;
int lastX = -1, lastY = -1;
unsigned long pressStart = 0, lastStatus = 0;

// =====================================================================
void centerText(const __FlashStringHelper *s, int y, uint8_t size, uint16_t c) {
  int16_t x1, y1; uint16_t w, h;
  tft.setTextSize(size);
  tft.getTextBounds(s, 0, y, &x1, &y1, &w, &h);
  tft.setCursor((tft.width() - (int)w) / 2, y);
  tft.setTextColor(c);
  tft.print(s);
}

void cross(int x, int y, uint16_t c) {
  tft.drawFastHLine(x - 12, y, 25, c);
  tft.drawFastVLine(x, y - 12, 25, c);
  tft.drawCircle(x, y, 6, c);
}

// ---------------- 1. diagnostics ----------------
void diagnostics() {
#if USE_ST7789
  Serial.println(F("\n[1] ST7789 mode - register read skipped"));
#else
  uint8_t mode   = tft.readcommand8(ILI9341_RDMODE);
  uint8_t madctl = tft.readcommand8(ILI9341_RDMADCTL);
  uint8_t pix    = tft.readcommand8(ILI9341_RDPIXFMT);
  uint8_t img    = tft.readcommand8(ILI9341_RDIMGFMT);
  uint8_t diag   = tft.readcommand8(ILI9341_RDSELFDIAG);
  Serial.println(F("\n[1] display registers"));
  Serial.print(F("    power mode   0x")); Serial.println(mode, HEX);
  Serial.print(F("    MADCTL       0x")); Serial.println(madctl, HEX);
  Serial.print(F("    pixel format 0x")); Serial.println(pix, HEX);
  Serial.print(F("    image format 0x")); Serial.println(img, HEX);
  Serial.print(F("    self diag    0x")); Serial.println(diag, HEX);
  if (pix == 0x00 || pix == 0xFF) {
    Serial.println(F("    -> no answer from the display."));
    Serial.println(F("       Screen shows the tests anyway? Then only SDO/MISO (D12) is missing - that's OK."));
    Serial.println(F("       Screen stays WHITE? Check VCC/GND, CS D10, DC D9, RESET D8, SCK D13, SDI D11 + dividers."));
  } else {
    Serial.println(F("    -> display answers (normal: pixel format 0x5, self diag 0xC0)"));
    Serial.println(F("       answers but stays white? try USE_ST7789 1"));
  }
#endif
}

// ---------------- 2. colour fills ----------------
void fillTest() {
  const uint16_t col[5] = { C_RED, C_GREEN, C_BLUE, C_WHITE, C_BLACK };
  const char name[5][6] = { "red", "green", "blue", "white", "black" };
  Serial.println(F("\n[2] colour fills - every colour must fill the WHOLE screen, no stripes"));
  for (int i = 0; i < 5; i++) {
    unsigned long t = millis();
    tft.fillScreen(col[i]);
    t = millis() - t;
    Serial.print(F("    ")); Serial.print(name[i]); Serial.print(F(" fill ")); Serial.print(t); Serial.println(F(" ms"));
    delay(500);
  }
}

// ---------------- 3. test picture ----------------
void patternTest() {
  Serial.println(F("\n[3] test picture - check:"));
  Serial.println(F("    - the thin white border shows on ALL 4 edges (whole 320x240 works)"));
  Serial.println(F("    - corners: red top-left, green top-right, blue bottom-left, yellow bottom-right"));
  Serial.println(F("    - 8 clean colour bars, smooth grey ramp, sharp text"));
  tft.fillScreen(C_BLACK);
  const uint16_t bars[8] = { C_WHITE, C_YELLOW, C_CYAN, C_GREEN,
                             C_MAGENTA, C_RED, C_BLUE, C_DARKGREY };
  for (int i = 0; i < 8; i++) tft.fillRect(i * 40, 34, 40, 70, bars[i]);
  for (int x = 0; x < W; x++) {
    uint8_t g = (uint32_t)x * 255 / (W - 1);
    tft.drawFastVLine(x, 110, 30, tft.color565(g, g, g));
  }
  tft.setTextColor(C_WHITE);
  tft.setTextSize(1); tft.setCursor(30, 150); tft.print(F("size 1: The quick brown fox 0123456789"));
  tft.setTextSize(2); tft.setCursor(30, 166); tft.print(F("size 2: NeuroX test"));
  tft.setTextColor(C_CYAN);
  centerText(F("NEUROX"), 192, 4, C_CYAN);
  tft.drawRect(0, 0, W, H, C_WHITE);               // 1 px border on the very edge
  tft.fillTriangle(1, 1, 26, 1, 1, 26, C_RED);
  tft.fillTriangle(W - 2, 1, W - 27, 1, W - 2, 26, C_GREEN);
  tft.fillTriangle(1, H - 2, 26, H - 2, 1, H - 27, C_BLUE);
  tft.fillTriangle(W - 2, H - 2, W - 27, H - 2, W - 2, H - 27, C_YELLOW);
  delay(5000);
}

// ---------------- 4. rotations ----------------
void rotationTest() {
  Serial.println(F("\n[4] rotations - the green arrow must point to the top edge of the text each time"));
  for (uint8_t r = 0; r < 4; r++) {
    tft.setRotation(r);
    tft.fillScreen(C_BLACK);
    tft.drawRect(0, 0, tft.width(), tft.height(), C_WHITE);
    int cx = tft.width() / 2;
    tft.fillTriangle(cx, 12, cx - 18, 42, cx + 18, 42, C_GREEN);
    tft.setTextSize(2); tft.setTextColor(C_WHITE);
    tft.setCursor(20, 60); tft.print(F("rotation ")); tft.print(r);
    tft.setCursor(20, 85); tft.print(tft.width()); tft.print(F(" x ")); tft.print(tft.height());
    delay(1500);
  }
  tft.setRotation(1);
}

// ---------------- 5. touch ----------------
// waits for a firm press, averages it, waits for release
bool readRaw(TS_Point &out) {
  unsigned long t0 = millis(); bool hinted = false;
  while (!ts.touched()) {
    if (!hinted && millis() - t0 > 10000) {
      hinted = true;
      Serial.println(F("    no touch detected yet. Check T_CS D4, T_CLK D13, T_DIN D11, T_DO D12, T_IRQ D3"));
      Serial.println(F("    (T_IRQ not connected? set TOUCH_IRQ to 255 and upload again)"));
    }
  }
  delay(30);
  long sx = 0, sy = 0, sz = 0; int n = 0;
  unsigned long t1 = millis();
  while (n < 16 && millis() - t1 < 700) {
    if (ts.touched()) {
      TS_Point p = ts.getPoint();
      if (p.z > Z_MIN) { sx += p.x; sy += p.y; sz += p.z; n++; }
    }
    delay(5);
  }
  while (ts.touched()) delay(10);                         // wait for release
  delay(150);
  if (n < 4) return false;                                // too light / too short
  out.x = sx / n; out.y = sy / n; out.z = sz / n;
  return true;
}

void mapPoint(const TS_Point &p, int &x, int &y) {
  long rx = calSwap ? p.y : p.x;
  long ry = calSwap ? p.x : p.y;
  x = constrain((int)(ax * rx + bx), 0, W - 1);
  y = constrain((int)(ay * ry + by), 0, H - 1);
}

// returns the centre error in pixels, or -1 if the calibration failed
int calibrate() {
  const int px[3] = { 20, W - 21, 20 };
  const int py[3] = { 20, 20, H - 21 };
  TS_Point r[3];
  Serial.println(F("\n[5] touch calibration - tap the CENTRE of each cross (fingernail or stylus)"));
  for (int i = 0; i < 3; i++) {
    tft.fillScreen(C_BLACK);
    centerText(F("TOUCH CALIBRATION"), 80, 2, C_WHITE);
    centerText(F("tap the centre of the cross"), 110, 1, C_LIGHTGREY);
    tft.setTextSize(2); tft.setTextColor(C_CYAN);
    tft.setCursor(W / 2 - 18, 140); tft.print(i + 1); tft.print(F("/3"));
    cross(px[i], py[i], C_WHITE);
    while (!readRaw(r[i])) {}
    Serial.print(F("    point ")); Serial.print(i + 1);
    Serial.print(F("  raw x=")); Serial.print(r[i].x);
    Serial.print(F(" y=")); Serial.print(r[i].y);
    Serial.print(F(" z=")); Serial.println(r[i].z);
  }
  // which raw axis moves when the screen x changes? (point 1 -> point 2)
  calSwap = abs(r[1].y - r[0].y) > abs(r[1].x - r[0].x);
  long s0x = calSwap ? r[0].y : r[0].x, s1x = calSwap ? r[1].y : r[1].x;
  long s0y = calSwap ? r[0].x : r[0].y, s2y = calSwap ? r[2].x : r[2].y;
  if (abs(s1x - s0x) < 300 || abs(s2y - s0y) < 300) {
    Serial.println(F("    FAILED: the raw values hardly changed - touch wiring, or you tapped the same spot"));
    tft.fillScreen(C_BLACK);
    centerText(F("calibration failed"), 100, 2, C_RED);
    centerText(F("see Serial Monitor - trying again"), 130, 1, C_WHITE);
    delay(2500);
    return -1;
  }
  ax = (float)(px[1] - px[0]) / (s1x - s0x); bx = px[0] - ax * s0x;
  ay = (float)(py[2] - py[0]) / (s2y - s0y); by = py[0] - ay * s0y;

  // check in the middle
  tft.fillScreen(C_BLACK);
  centerText(F("CHECK: tap the yellow cross"), 40, 1, C_WHITE);
  cross(W / 2, H / 2, C_YELLOW);
  TS_Point v; while (!readRaw(v)) {}
  int mx, my; mapPoint(v, mx, my);
  int err = (int)sqrt((float)(mx - W / 2) * (mx - W / 2) + (float)(my - H / 2) * (my - H / 2));
  tft.fillCircle(mx, my, 4, C_RED);
  tft.setTextSize(2); tft.setCursor(70, 180);
  tft.setTextColor(err <= 15 ? C_GREEN : C_RED);
  tft.print(F("error ")); tft.print(err); tft.print(err <= 15 ? F(" px  OK") : F(" px  BAD"));

  Serial.print(F("    centre check: error ")); Serial.print(err); Serial.println(err <= 15 ? F(" px -> OK") : F(" px -> too big, again"));
  Serial.print(F("    calibration: swap=")); Serial.print(calSwap);
  Serial.print(F(" ax=")); Serial.print(ax, 5); Serial.print(F(" bx=")); Serial.print(bx, 1);
  Serial.print(F(" ay=")); Serial.print(ay, 5); Serial.print(F(" by=")); Serial.println(by, 1);
  delay(1800);
  return err;
}

void doCalibration() {
  for (int tries = 0; tries < 3; tries++) {
    int e = calibrate();
    if (e >= 0 && e <= 15) return;
  }
  Serial.println(F("    3 tries without a good check - continuing with the last calibration"));
}

// ---------------- 6. paint mode ----------------
void drawToolbar() {
  for (int i = 0; i < 6; i++) {
    tft.fillRect(i * 40, 0, 40, BAR, PAL[i]);
    if (PAL[i] == ink) { tft.drawRect(i * 40 + 1, 1, 38, BAR - 2, C_BLACK); tft.drawRect(i * 40 + 2, 2, 36, BAR - 4, C_BLACK); }
  }
  tft.fillRect(240, 0, 80, BAR, C_DARKGREY);
  tft.setTextSize(2); tft.setTextColor(C_WHITE); tft.setCursor(251, 7); tft.print(F("CLEAR"));
  tft.drawFastHLine(0, BAR, W, C_DARKGREY);
}

void drawStatus(int x, int y, const TS_Point &p) {
  char buf[54];
  snprintf(buf, sizeof(buf), "x=%3d y=%3d   raw x=%4d y=%4d z=%4d   ", x, y, p.x, p.y, p.z);
  tft.setTextSize(1); tft.setTextColor(C_WHITE, C_NAVY);
  tft.setCursor(4, H - STAT + 2); tft.print(buf);
}

void paintScreen() {
  tft.fillScreen(C_BLACK);
  drawToolbar();
  tft.fillRect(0, H - STAT, W, STAT, C_NAVY);
  tft.setTextSize(1); tft.setTextColor(C_WHITE);
  tft.setCursor(4, H - STAT + 2); tft.print(F("draw! hold CLEAR 3 s = calibrate again"));
  Serial.println(F("\n[6] paint mode - the line must appear right under your finger, also in the corners"));
}

// =====================================================================
void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println(F("\nsketch running - starting the screen..."));
  pinMode(TOUCH_CS, OUTPUT); digitalWrite(TOUCH_CS, HIGH);   // keep the touch chip quiet during init
  pinMode(TFT_CS, OUTPUT);   digitalWrite(TFT_CS, HIGH);
#if USE_ST7789
  tft.init(240, 320);
  if (SPI_HZ) tft.setSPISpeed(SPI_HZ);
#else
  tft.begin(SPI_HZ);
#endif
  ts.begin();
  Serial.println(F("\n=== NeuroX display + touch test (UNO) ==="));
  diagnostics();
  tft.setRotation(1);
  fillTest();
  patternTest();
  rotationTest();

  tft.fillScreen(C_BLACK);
  centerText(F("DISPLAY TEST DONE"), 70, 2, C_GREEN);
  centerText(F("tap the screen to start"), 110, 2, C_WHITE);
  centerText(F("the touch test"), 135, 2, C_WHITE);
  Serial.println(F("\ntap the screen to start the touch test..."));
  TS_Point first; while (!readRaw(first)) {}
  Serial.print(F("    first touch raw x=")); Serial.print(first.x); Serial.print(F(" y=")); Serial.print(first.y);
  Serial.print(F(" z=")); Serial.println(first.z);

  doCalibration();
  paintScreen();
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'c' || c == 'C') { doCalibration(); paintScreen(); down = false; return; }
  }
  if (!ts.touched()) { down = false; lastX = -1; return; }
  TS_Point p = ts.getPoint();
  if (p.z < Z_MIN) return;
  int x, y; mapPoint(p, x, y);
  bool newPress = !down;
  if (newPress) pressStart = millis();
  down = true;

  if (y < BAR) {                                            // toolbar
    if (x >= 240) {
      if (newPress) { tft.fillRect(0, BAR + 1, W, H - BAR - STAT - 1, C_BLACK); Serial.println(F("    CLEAR")); }
      if (millis() - pressStart > 3000) {
        while (ts.touched()) delay(10);
        doCalibration(); paintScreen(); down = false; return;
      }
    } else if (newPress) {
      ink = PAL[x / 40]; drawToolbar();
    }
    lastX = -1;
  } else if (y < H - STAT - 3) {                            // canvas
    if (lastX >= 0 && abs(x - lastX) < 30 && abs(y - lastY) < 30 && lastY > BAR + 3) tft.drawLine(lastX, lastY, x, y, ink);
    tft.fillCircle(x, y, 2, ink);
    lastX = x; lastY = y;
  }
  if (millis() - lastStatus > 100) {
    lastStatus = millis();
    drawStatus(x, y, p);
    Serial.print(F("touch x=")); Serial.print(x); Serial.print(F(" y=")); Serial.print(y);
    Serial.print(F("  raw ")); Serial.print(p.x); Serial.print(F(",")); Serial.print(p.y);
    Serial.print(F(" z=")); Serial.println(p.z);
  }
}
