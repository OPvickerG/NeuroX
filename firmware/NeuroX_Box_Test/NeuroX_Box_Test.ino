/*
 * ============================================================================
 *  NeuroX Box Test  v3.1  -  check every part of the box, no HALO needed
 * ============================================================================
 *  Flash this to the box ESP32-S3, then follow the screen. 8 steps:
 *    1 SCREEN    colours, text, corners, backlight (GPIO 14)
 *    2 BUTTONS   press LEFT, NEW, OK and STOP once each
 *    3 TOUCH     tap the 4 targets -> prints the calibration lines for NeuroX_Box_UI
 *    4 BUZZER    beeps on GPIO 13
 *    5 MPU-6050  I2C on SDA 41 / SCL 42, live tilt
 *    6 POWER     battery voltage (GPIO 2) + USB-C in the CHARGE slot (GPIO 21)
 *    7 RADIO     sends F / R / S to the wheelchair  (WHEELS OFF THE GROUND)
 *    8 REPORT    pass / fail list, also printed on the Serial monitor
 *
 *  Buttons on most pages:  LEFT / NEW / OK = the labels at the bottom of the screen.
 *  STOP always sends 'S' to the chair.
 *  Serial monitor 115200:  n next   p previous   1-8 jump to a step   r report   w = try the next rotation
 *
 *  BOARD: "ESP32S3 Dev Module".  LIBRARIES: Adafruit GFX, Adafruit ST7735 and ST7789 Library
 *  (or Adafruit ILI9341), Adafruit BusIO, XPT2046_Touchscreen.  Same pins as NeuroX_Box_UI.
 *  Use the same TFT_ROTATION as in NeuroX_Box_UI, so the touch numbers match.
 * ============================================================================
 */
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Adafruit_GFX.h>
#include <XPT2046_Touchscreen.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>

// ---------------- settings ----------------
// ---- screen chip. Your TPM408-2.8 board is an ST7789: driven as an ILI9341 it stays portrait
//      with red and blue swapped (the ILI9341 start-up writes 0xC0, which on the ST7789 sets
//      hidden flip bits that cancel the landscape bit).
#define TFT_DRIVER        7789    // 7789 = ST7789 (TPM408-2.8 boards)   9341 = ILI9341
#define TFT_INVERT        0       // ST7789: colours look like a photo negative? set 1
#define TFT_SWAP_RB       0       // red shows blue and blue shows red? set 1
#if TFT_DRIVER == 7789
#include <Adafruit_ST7789.h>
#else
#include <Adafruit_ILI9341.h>
#endif
#define TFT_ROTATION   3              // same as NeuroX_Box_UI
#define TFT_SPI_HZ     20000000UL     // 20 MHz is safe on a 15 cm ribbon; short wires can go 40000000UL
#define TOUCH_ROTATION TFT_ROTATION

// ---------------- pins (BOX S3 BOARD WIRING) ----------------
static const int8_t PIN_TFT_SCK = 12, PIN_TFT_MOSI = 11, PIN_TFT_MISO = 10;
static const int8_t PIN_TFT_CS = 9, PIN_TFT_DC = 8, PIN_TFT_RST = 18, PIN_TFT_LED = 14;
static const int8_t PIN_T_CS = 17, PIN_T_IRQ = 16;
static const int8_t PIN_BTN_LEFT = 6, PIN_BTN_NEW = 7, PIN_BTN_OK = 5, PIN_BTN_STOP = 15;
static const int8_t PIN_BUZZER = 13, PIN_SDA = 41, PIN_SCL = 42, PIN_VBAT = 2, PIN_USB_SENSE = 21;

// ---------------- colours + types ----------------
#define RGB(r, g, b) ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))
static const uint16_t C_BG = RGB(10, 13, 19), C_PANEL = RGB(21, 26, 36), C_LINE = RGB(42, 50, 66), C_FG = RGB(233, 237, 245);
static const uint16_t C_MUT = RGB(127, 138, 158), C_BLUE = RGB(47, 123, 255), C_GREEN = RGB(43, 208, 123), C_AMBER = RGB(255, 176, 32);
static const uint16_t C_RED = RGB(255, 59, 59), C_WHITE = 0xFFFF, C_BLACK = 0x0000;

enum Page : uint8_t { PG_SCREEN, PG_BUTTONS, PG_TOUCH, PG_BUZZER, PG_MPU, PG_POWER, PG_RADIO, PG_REPORT, PG_N };
enum Res  : uint8_t { R_NONE, R_PASS, R_FAIL };
enum Key  : uint8_t { K_LEFT, K_NEW, K_OK, K_STOP };
enum Fnt  : uint8_t { FT_TINY, FT_SM, FT_MD, FT_LG };
enum Align : uint8_t { AL_L, AL_C, AL_R };
static const char *PAGE_NAME[PG_N] = {"Screen", "Buttons", "Touch", "Buzzer", "MPU-6050", "Power", "Radio", "Report"};

#if TFT_DRIVER == 7789
static Adafruit_ST7789 tft(&SPI, PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST);     // note: CS, DC, RST
#else
static Adafruit_ILI9341 tft(&SPI, PIN_TFT_DC, PIN_TFT_CS, PIN_TFT_RST);    // note: DC, CS, RST
#endif
static XPT2046_Touchscreen ts(PIN_T_CS, PIN_T_IRQ);

static Page page = PG_SCREEN;
static uint8_t rot = TFT_ROTATION;
static Res  result[PG_N];
static char note[PG_N][40];
static uint32_t pageMs = 0, liveMs = 0;
static const int16_t SOFT_X[3] = {38, 170, 300};          // right above LEFT / NEW / OK on the lid

// buttons
struct Btn { int8_t pin; Key key; const char *name; bool down; uint32_t t; bool seen; bool stuck; };
static Btn BTNS[4] = {{PIN_BTN_LEFT, K_LEFT, "LEFT", false, 0, false, false}, {PIN_BTN_NEW, K_NEW, "NEW", false, 0, false, false},
                      {PIN_BTN_OK, K_OK, "OK", false, 0, false, false}, {PIN_BTN_STOP, K_STOP, "STOP", false, 0, false, false}};
// touch
static const int16_t TGT_X[4] = {24, 296, 24, 296}, TGT_Y[4] = {40, 40, 206, 206};
static int16_t tRawX[4], tRawY[4]; static uint8_t tStep = 0; static bool tWas = false; static uint32_t tMs = 0;
static int32_t calMinX = 300, calMaxX = 3800, calMinY = 300, calMaxY = 3800; static bool calSwap = false, calDone = false;
static int16_t lastDotX = -1, lastDotY = -1;
// buzzer
static const uint32_t BUZZ_FREQ_HZ = 2048, BUZZ_DUTY = 128; static const uint8_t BUZZ_LEDC_CH = 2;
static uint32_t buzzOffAt = 0, buzzNextAt = 0; static bool buzzing = false;
// mpu
static bool mpuFound = false; static uint8_t mpuWho = 0; static float ax = 0, ay = 0, az = 0, tempC = 0, pitch = 0, roll = 0;
static char i2cList[48] = "";
// power
static float batMv = 0, pinMv = 0; static bool usb = false, usbSeenHi = false, usbSeenLo = false;
// radio
static const uint8_t BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
struct __attribute__((packed)) CarPacket { uint8_t magic0, magic1, cmd, seq; };
static bool radioReady = false; static char radioCmd = 'S'; static uint8_t radioSeq = 0, burst = 0;
static uint32_t radioLast = 0, radioChange = 0, pulseEnd = 0, txOk = 0, txFail = 0, pulsesSent = 0;

static void screenBegin() {
#if TFT_DRIVER == 7789
  tft.init(240, 320); tft.setSPISpeed(TFT_SPI_HZ); tft.invertDisplay(TFT_INVERT);
#else
  tft.begin(TFT_SPI_HZ);
#endif
}
static uint8_t madctlFor(uint8_t r) {            // what setRotation(r) writes, for the chosen chip
#if TFT_DRIVER == 7789
  static const uint8_t M[4] = {0xC0, 0xA0, 0x00, 0x60};
#else
  static const uint8_t M[4] = {0x48, 0x28, 0x88, 0xE8};
#endif
  return M[r & 3];
}
static void setRot(uint8_t r) {
  tft.setRotation(r);
  if (TFT_SWAP_RB) { uint8_t m = madctlFor(r) ^ 0x08; tft.sendCommand(0x36, &m, 1); }   // 0x36 = MADCTL on both chips
}
// ============================================================================
// drawing helpers (direct to the screen)
// ============================================================================
static const GFXfont *fontOf(Fnt f) { return f == FT_SM ? &FreeSans9pt7b : f == FT_MD ? &FreeSansBold9pt7b : f == FT_LG ? &FreeSansBold12pt7b : nullptr; }
static int16_t textW(const char *s, Fnt f) {
  if (f == FT_TINY) return (int16_t)(strlen(s) * 6);
  tft.setFont(fontOf(f)); int16_t x1, y1; uint16_t w, h; tft.getTextBounds(s, 0, 0, &x1, &y1, &w, &h); return (int16_t)(x1 + w);
}
static void text(const char *s, int16_t x, int16_t y, Fnt f, uint16_t col, Align a = AL_L) {   // y = baseline
  const int16_t w = a == AL_L ? 0 : textW(s, f);
  tft.setFont(fontOf(f)); tft.setTextSize(1); tft.setTextColor(col); tft.setTextWrap(false);
  tft.setCursor(a == AL_L ? x : a == AL_C ? x - w / 2 : x - w, f == FT_TINY ? y - 7 : y); tft.print(s);
}
static void clearBody() { tft.fillRect(0, 21, 320, 200, C_BG); }
static void header() {
  tft.fillRect(0, 0, 320, 20, C_PANEL);
  char b[32]; snprintf(b, sizeof(b), "BOX TEST  %u/8", (unsigned)page + 1);
  text(b, 6, 15, FT_MD, C_BLUE); text(PAGE_NAME[page], 140, 15, FT_MD, C_FG);
  for (uint8_t i = 0; i < PG_REPORT; ++i)
    tft.fillCircle(240 + i * 11, 10, 4, result[i] == R_PASS ? C_GREEN : result[i] == R_FAIL ? C_RED : (i == page ? C_FG : C_LINE));
}
static void softKeys(const char *a, const char *b, const char *c) {
  tft.fillRect(0, 222, 320, 18, C_PANEL);
  if (a && *a) text(a, SOFT_X[0], 236, FT_SM, C_BLUE, AL_C);
  if (b && *b) text(b, SOFT_X[1], 236, FT_SM, C_FG, AL_C);
  if (c && *c) text(c, 314, 236, FT_SM, C_BLUE, AL_R);
}
static void valueBox(int16_t x, int16_t y, int16_t w, const char *s, Fnt f, uint16_t col, Align a = AL_L) {   // redraw one live value
  const int16_t up = f == FT_LG ? 20 : f == FT_TINY ? 9 : 15;
  tft.fillRect(x, y - up, w, up + 6, C_BG);
  text(s, a == AL_L ? x : a == AL_C ? x + w / 2 : x + w, y, f, col, a);
}
static void setResult(Page p, Res r, const char *why) {
  result[p] = r; strncpy(note[p], why ? why : "", sizeof(note[p]) - 1); note[p][sizeof(note[p]) - 1] = 0;
  Serial.printf("[%s] %s %s\n", PAGE_NAME[p], r == R_PASS ? "PASS" : r == R_FAIL ? "FAIL" : "-", note[p]);
}

// ============================================================================
// buzzer + radio
// ============================================================================
static void buzz(bool on) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
  ledcWrite(PIN_BUZZER, on ? BUZZ_DUTY : 0);
#else
  ledcWrite(BUZZ_LEDC_CH, on ? BUZZ_DUTY : 0);
#endif
}
static void beep(uint16_t ms) { buzz(true); buzzOffAt = millis() + ms; buzzing = true; }
static void radioInit() {
  WiFi.mode(WIFI_STA); WiFi.disconnect();
  esp_wifi_set_ps(WIFI_PS_NONE); esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE); esp_wifi_set_max_tx_power(60);
  if (esp_now_init() != ESP_OK) return;
  esp_now_peer_info_t peer; memset(&peer, 0, sizeof(peer));
  memcpy(peer.peer_addr, BROADCAST_MAC, 6); peer.channel = 1; peer.ifidx = WIFI_IF_STA; peer.encrypt = false;
  radioReady = esp_now_add_peer(&peer) == ESP_OK;
}
static void radioSend(char c) { radioCmd = c; burst = 3; radioChange = millis(); }
static void radioService() {           // same heartbeat as the real box: 25 ms after a change, then 80 ms
  if (!radioReady) return;
  const uint32_t now = millis();
  if (radioCmd != 'S' && (int32_t)(now - pulseEnd) >= 0) radioSend('S');
  const uint32_t period = (now - radioChange < 250) ? 25 : 80;
  if (burst == 0 && now - radioLast < period) return;
  CarPacket p = {'N', 'X', (uint8_t)radioCmd, radioSeq};
  if (esp_now_send(BROADCAST_MAC, (const uint8_t *)&p, sizeof(p)) == ESP_OK) { ++radioSeq; ++txOk; if (burst) --burst; radioLast = now; } else ++txFail;
}
static void pulse(char c) { radioSend(c); pulseEnd = millis() + 420; ++pulsesSent; beep(60); Serial.printf("[RADIO] sent %c for 420 ms\n", c); }

// ============================================================================
// pages: enter() draws the page, live() updates moving parts
// ============================================================================
static void enterPage(Page p);
static void nextPage() { enterPage((Page)min<int>(page + 1, PG_REPORT)); }

// ---- 1 SCREEN
static void drawScreenPage() {
  const uint16_t bars[6] = {C_RED, RGB(0, 255, 0), RGB(0, 0, 255), C_WHITE, RGB(128, 128, 128), C_BLACK};
  const char *nm[6] = {"RED", "GREEN", "BLUE", "WHITE", "GREY", "BLACK"};
  for (uint8_t i = 0; i < 6; ++i) { tft.fillRect(10 + i * 50, 28, 48, 44, bars[i]); text(nm[i], 34 + i * 50, 84, FT_TINY, C_MUT, AL_C); }
  tft.drawRect(260, 28, 48, 44, C_LINE);
  for (uint8_t i = 0; i < 32; ++i) tft.fillRect(10 + i * 9, 92, 9, 12, RGB(i * 8, i * 8, i * 8));
  text("Can you read this clearly?", 160, 128, FT_MD, C_FG, AL_C);
  text("Colours in the right order, no stripes?", 160, 148, FT_SM, C_MUT, AL_C);
  text("red/blue swapped: TFT_SWAP_RB 1   negative: TFT_INVERT 1", 160, 160, FT_TINY, C_MUT, AL_C);
  for (uint8_t i = 0; i < 3; ++i) {                       // markers that must sit right above the lid buttons
    const int16_t x = i == 2 ? 300 : SOFT_X[i];
    tft.fillTriangle(x - 7, 204, x + 7, 204, x, 214, C_AMBER);
    text(i == 0 ? "LEFT" : i == 1 ? "NEW" : "OK", x, 198, FT_TINY, C_AMBER, AL_C);
  }
  text("these must sit right above the buttons", 160, 172, FT_TINY, C_AMBER, AL_C);
  // corner marks: all 4 must be visible (picture not shifted or cut off)
  const uint16_t c = C_GREEN;
  tft.drawFastHLine(0, 239, 12, c); tft.drawFastVLine(0, 228, 12, c); tft.drawFastHLine(308, 239, 12, c); tft.drawFastVLine(319, 228, 12, c);
  tft.drawFastHLine(0, 21, 12, c); tft.drawFastVLine(0, 21, 12, c); tft.drawFastHLine(308, 21, 12, c); tft.drawFastVLine(319, 21, 12, c);
  softKeys("bad", "backlight", "good");
}
// ---- 2 BUTTONS
static void drawBtnTile(uint8_t i) {
  const Btn &b = BTNS[i];
  int16_t x, y, w = 76, h = 56;
  if (i == 3) { x = 222; y = 34; } else { x = SOFT_X[i] - w / 2; if (i == 2) x = 300 - w + 10; y = 140; }
  const uint16_t fill = b.stuck ? C_RED : b.down ? C_BLUE : C_PANEL, edge = b.seen ? C_GREEN : C_LINE;
  tft.fillRoundRect(x, y, w, h, 8, fill); tft.drawRoundRect(x, y, w, h, 8, edge); tft.drawRoundRect(x + 1, y + 1, w - 2, h - 2, 7, edge);
  text(b.name, x + w / 2, y + 26, FT_MD, b.down ? C_WHITE : C_FG, AL_C);
  char g[16];
  if (b.stuck) snprintf(g, sizeof(g), "HELD / SHORT?"); else if (b.seen) snprintf(g, sizeof(g), "OK  GPIO %d", b.pin); else snprintf(g, sizeof(g), "GPIO %d", b.pin);
  text(g, x + w / 2, y + 44, FT_TINY, b.stuck ? C_WHITE : b.seen ? C_GREEN : C_MUT, AL_C);
}
static void drawButtonsPage() {
  text("Press each button once", 12, 46, FT_MD, C_FG);
  text("tiles turn blue while held,", 12, 70, FT_SM, C_MUT);
  text("green edge = seen", 12, 90, FT_SM, C_MUT);
  text("STOP is the red one, top right", 12, 120, FT_TINY, C_MUT);
  for (uint8_t i = 0; i < 4; ++i) drawBtnTile(i);
  softKeys("", "(serial n = skip)", "");
}
// ---- 3 TOUCH
static void drawTarget(uint8_t i, uint16_t col) {
  const int16_t x = TGT_X[i], y = TGT_Y[i];
  tft.drawCircle(x, y, 10, col); tft.drawCircle(x, y, 9, col); tft.drawFastHLine(x - 16, y, 33, col); tft.drawFastVLine(x, y - 16, 33, col);
}
static void drawTouchPage() {
  tStep = 0; calDone = false; lastDotX = -1;
  for (uint8_t i = 0; i < 4; ++i) drawTarget(i, C_LINE);
  drawTarget(0, C_AMBER);
  text("Tap the orange target", 160, 112, FT_MD, C_FG, AL_C);
  text("use a fingernail or a stylus, tap the centre", 160, 134, FT_TINY, C_MUT, AL_C);
  softKeys("redo", "skip", "");
}
static void computeTouchCal() {
  // raw readings across the screen's x direction: if raw Y changes more than raw X, the axes are swapped
  const float dxX = ((tRawX[1] + tRawX[3]) - (tRawX[0] + tRawX[2])) / 2.0f, dxY = ((tRawY[1] + tRawY[3]) - (tRawY[0] + tRawY[2])) / 2.0f;
  calSwap = fabsf(dxY) > fabsf(dxX);
  int16_t rx[4], ry[4];
  for (uint8_t i = 0; i < 4; ++i) { rx[i] = calSwap ? tRawY[i] : tRawX[i]; ry[i] = calSwap ? tRawX[i] : tRawY[i]; }
  const float xl = (rx[0] + rx[2]) / 2.0f, xr = (rx[1] + rx[3]) / 2.0f, yt = (ry[0] + ry[1]) / 2.0f, yb = (ry[2] + ry[3]) / 2.0f;
  const float sx = (xr - xl) / (TGT_X[1] - TGT_X[0]), sy = (yb - yt) / (TGT_Y[2] - TGT_Y[0]);
  calMinX = constrain((int32_t)(xl - TGT_X[0] * sx), 0, 4095); calMaxX = constrain((int32_t)(xl + (320 - TGT_X[0]) * sx), 0, 4095);
  calMinY = constrain((int32_t)(yt - TGT_Y[0] * sy), 0, 4095); calMaxY = constrain((int32_t)(yt + (240 - TGT_Y[0]) * sy), 0, 4095);
  const bool sane = fabsf(xr - xl) > 400 && fabsf(yb - yt) > 300;
  calDone = true;
  Serial.println("\n---- paste these into the top of NeuroX_Box_UI.ino ----");
  Serial.printf("static const uint16_t TS_MINX = %ld, TS_MAXX = %ld, TS_MINY = %ld, TS_MAXY = %ld;\n", (long)calMinX, (long)calMaxX, (long)calMinY, (long)calMaxY);
  Serial.printf("#define TOUCH_SWAP_XY     %d\n#define TOUCH_FLIP_X      0\n#define TOUCH_FLIP_Y      0\n", calSwap ? 1 : 0);
  Serial.println("-------------------------------------------------------");
  setResult(PG_TOUCH, sane ? R_PASS : R_FAIL, sane ? (calSwap ? "calibrated (axes swapped)" : "calibrated") : "taps too close: redo");
  clearBody(); header();
  text(sane ? "Touch calibrated" : "Readings look wrong", 160, 50, FT_MD, sane ? C_GREEN : C_RED, AL_C);
  char b[64];
  snprintf(b, sizeof(b), "X %ld..%ld   Y %ld..%ld", (long)calMinX, (long)calMaxX, (long)calMinY, (long)calMaxY); text(b, 160, 74, FT_SM, C_FG, AL_C);
  snprintf(b, sizeof(b), "swap X/Y: %s", calSwap ? "YES" : "no"); text(b, 160, 96, FT_SM, C_MUT, AL_C);
  text("the lines are on the Serial monitor", 160, 116, FT_TINY, C_MUT, AL_C);
  text("tap anywhere: the dot must land under it", 160, 136, FT_TINY, C_AMBER, AL_C);
  softKeys("redo", "", "next");
}
static void touchLive() {
  if (millis() - tMs < 25) return; tMs = millis();
  const bool t = ts.touched();
  if (t && !tWas) {
    delay(15);                                           // let the reading settle, then average 4
    int32_t sx = 0, sy = 0; uint8_t n = 0;
    for (uint8_t k = 0; k < 4; ++k) { TS_Point p = ts.getPoint(); if (p.z > 100) { sx += p.x; sy += p.y; ++n; } delay(3); }
    if (n) {
      const int16_t rx = (int16_t)(sx / n), ry = (int16_t)(sy / n);
      Serial.printf("[TOUCH] raw %d, %d\n", rx, ry);
      if (!calDone) {
        tRawX[tStep] = rx; tRawY[tStep] = ry; beep(30);
        drawTarget(tStep, C_GREEN);
        if (++tStep < 4) drawTarget(tStep, C_AMBER); else computeTouchCal();
      } else {
        const int16_t ux = calSwap ? ry : rx, uy = calSwap ? rx : ry;
        int16_t x = (int16_t)constrain(map(ux, calMinX, calMaxX, 0, 320), 0L, 319L), y = (int16_t)constrain(map(uy, calMinY, calMaxY, 0, 240), 22L, 220L);
        if (lastDotX >= 0) tft.fillCircle(lastDotX, lastDotY, 6, C_BG);
        tft.fillCircle(x, y, 6, C_BLUE); lastDotX = x; lastDotY = y;
      }
    }
  }
  tWas = t;
}
// ---- 4 BUZZER
static void drawBuzzerPage() {
  text("Do you hear a beep", 160, 70, FT_LG, C_FG, AL_C);
  text("every second?", 160, 96, FT_LG, C_FG, AL_C);
  text("2048 Hz, the same tone as the real box", 160, 126, FT_TINY, C_MUT, AL_C);
  text("silent? check BZ+ on GPIO 13 and BZ- on GND,", 160, 160, FT_TINY, C_MUT, AL_C);
  text("and the + / - way round", 160, 174, FT_TINY, C_MUT, AL_C);
  softKeys("silent", "long beep", "I hear it");
  buzzNextAt = millis();
}
static void buzzerLive() { if ((int32_t)(millis() - buzzNextAt) >= 0) { beep(150); buzzNextAt = millis() + 1000; } }
// ---- 5 MPU-6050
static bool mpuWrite(uint8_t reg, uint8_t v) { Wire.beginTransmission(0x68); Wire.write(reg); Wire.write(v); return Wire.endTransmission() == 0; }
static bool mpuRead(uint8_t reg, uint8_t *buf, uint8_t n) {
  Wire.beginTransmission(0x68); Wire.write(reg); if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(0x68, (int)n) != n) return false;
  for (uint8_t i = 0; i < n; ++i) buf[i] = (uint8_t)Wire.read();
  return true;
}
static void mpuScan() {
  i2cList[0] = 0; uint8_t found = 0;
  for (uint8_t a = 1; a < 127; ++a) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0 && strlen(i2cList) < sizeof(i2cList) - 6) { char b[6]; snprintf(b, sizeof(b), "0x%02X ", a); strcat(i2cList, b); ++found; }
  }
  if (!found) strcpy(i2cList, "nothing");
  uint8_t w = 0; mpuFound = mpuWrite(0x6B, 0x00) && mpuRead(0x75, &w, 1); mpuWho = w;
  Serial.printf("[MPU] I2C bus: %s  WHO_AM_I 0x%02X\n", i2cList, mpuWho);
}
static void drawMpuPage() {
  mpuScan();
  char b[64];
  snprintf(b, sizeof(b), "I2C found: %s", i2cList); text(b, 12, 44, FT_SM, C_FG);
  if (!mpuFound) {
    text("No MPU-6050 at 0x68", 12, 76, FT_MD, C_RED);
    text("check: VCC to 3V3, GND, SDA to GPIO 41,", 12, 100, FT_TINY, C_MUT);
    text("SCL to GPIO 42, AD0 open or to GND", 12, 114, FT_TINY, C_MUT);
    setResult(PG_MPU, R_FAIL, "not found at 0x68");
  } else {
    snprintf(b, sizeof(b), "WHO_AM_I 0x%02X %s", mpuWho, mpuWho == 0x68 ? "(MPU-6050)" : "(a clone, still works)");
    text(b, 12, 66, FT_SM, mpuWho == 0x68 ? C_GREEN : C_AMBER);
    text("tilt the box: the dot moves", 12, 210, FT_TINY, C_MUT);
    tft.drawCircle(250, 132, 44, C_LINE); tft.drawCircle(250, 132, 12, C_LINE);
  }
  softKeys("", "scan again", "next");
}
static void mpuLive() {
  if (!mpuFound || millis() - liveMs < 120) return; liveMs = millis();
  uint8_t d[14]; if (!mpuRead(0x3B, d, 14)) { valueBox(12, 100, 170, "read failed", FT_SM, C_RED); return; }
  const int16_t rx = (int16_t)((d[0] << 8) | d[1]), ry = (int16_t)((d[2] << 8) | d[3]), rz = (int16_t)((d[4] << 8) | d[5]), rt = (int16_t)((d[6] << 8) | d[7]);
  ax = rx / 16384.0f; ay = ry / 16384.0f; az = rz / 16384.0f; tempC = rt / 340.0f + 36.53f;
  pitch = atan2f(ax, sqrtf(ay * ay + az * az)) * 57.2958f; roll = atan2f(ay, az) * 57.2958f;
  const float g = sqrtf(ax * ax + ay * ay + az * az);
  char b[40];
  snprintf(b, sizeof(b), "x %+.2f  y %+.2f  z %+.2f g", ax, ay, az); valueBox(12, 100, 180, b, FT_TINY, C_FG);
  snprintf(b, sizeof(b), "total %.2f g", g); valueBox(12, 124, 180, b, FT_SM, (g > 0.8f && g < 1.2f) ? C_GREEN : C_AMBER);
  snprintf(b, sizeof(b), "pitch %+.0f  roll %+.0f", pitch, roll); valueBox(12, 150, 180, b, FT_SM, C_FG);
  snprintf(b, sizeof(b), "chip %.0f C", tempC); valueBox(12, 176, 180, b, FT_SM, C_MUT);
  if (lastDotX >= 0) tft.fillCircle(lastDotX, lastDotY, 8, C_BG);
  tft.drawCircle(250, 132, 44, C_LINE); tft.drawCircle(250, 132, 12, C_LINE);
  lastDotX = (int16_t)(250 + constrain(roll, -30.0f, 30.0f) * 1.2f); lastDotY = (int16_t)(132 + constrain(pitch, -30.0f, 30.0f) * 1.2f);
  tft.fillCircle(lastDotX, lastDotY, 8, C_GREEN);
  if (result[PG_MPU] == R_NONE && g > 0.8f && g < 1.2f) setResult(PG_MPU, R_PASS, mpuWho == 0x68 ? "1 g, live" : "1 g, live (clone chip)");
}
// ---- 6 POWER
static void drawPowerPage() {
  usbSeenHi = usbSeenLo = false;
  text("Battery  (rocker ON)", 12, 44, FT_MD, C_FG);
  text("USB-C in the CHARGE slot", 12, 142, FT_MD, C_FG);
  text("plug a cable in, then pull it out", 12, 158, FT_TINY, C_MUT);
  softKeys("", "", "next");
}
static uint8_t lipoPct(float mv) {
  static const float V[] = {3300, 3500, 3600, 3700, 3800, 3900, 4000, 4100, 4200}, P[] = {0, 5, 15, 30, 50, 65, 80, 90, 100};
  if (mv <= V[0]) return 0; if (mv >= V[8]) return 100;
  for (int i = 1; i < 9; ++i) if (mv < V[i]) return (uint8_t)(P[i - 1] + (P[i] - P[i - 1]) * (mv - V[i - 1]) / (V[i] - V[i - 1]));
  return 100;
}
static bool batOk() { return batMv > 3200 && batMv < 4350; }
static void powerLive() {
  if (millis() - liveMs < 250) return; liveMs = millis();
  uint32_t s = 0; for (uint8_t i = 0; i < 8; ++i) s += analogReadMilliVolts(PIN_VBAT);
  pinMv = s / 8.0f; batMv = pinMv * 2.0f;                  // 100k / 100k divider
  usb = digitalRead(PIN_USB_SENSE) == HIGH; if (usb) usbSeenHi = true; else usbSeenLo = true;
  char b[48];
  snprintf(b, sizeof(b), "%.2f V  %u%%", batMv / 1000.0f, (unsigned)lipoPct(batMv)); valueBox(12, 76, 200, b, FT_LG, batOk() ? C_GREEN : C_RED);
  snprintf(b, sizeof(b), "GPIO 2 reads %.0f mV (half of it)", pinMv); valueBox(12, 96, 300, b, FT_TINY, C_MUT);
  const char *why = batMv < 500 ? "0 V: R1/R2 or the VBAT wire to GPIO 2 missing" : batMv < 3200 ? "low: charge it, or rocker OFF?" : batMv > 4350 ? "too high: check R1 = R2 = 100k" : "in range";
  valueBox(12, 112, 300, why, FT_TINY, batOk() ? C_MUT : C_AMBER);
  snprintf(b, sizeof(b), "now: %s", usb ? "USB IN (GPIO 21 HIGH)" : "no USB (GPIO 21 LOW)"); valueBox(12, 184, 300, b, FT_SM, usb ? C_AMBER : C_FG);
  snprintf(b, sizeof(b), "seen plugged %s   seen unplugged %s", usbSeenHi ? "YES" : "no", usbSeenLo ? "YES" : "no");
  valueBox(12, 206, 300, b, FT_TINY, (usbSeenHi && usbSeenLo) ? C_GREEN : C_MUT);
}
// ---- 7 RADIO
static void drawRadioPage() {
  tft.fillRoundRect(10, 28, 300, 34, 6, C_RED);
  text("WHEELS OFF THE GROUND", 160, 51, FT_MD, C_WHITE, AL_C);
  char b[64]; snprintf(b, sizeof(b), "ESP-NOW ch 1: %s", radioReady ? "ready" : "FAILED"); text(b, 12, 84, FT_SM, radioReady ? C_GREEN : C_RED);
  snprintf(b, sizeof(b), "box MAC %s", WiFi.macAddress().c_str()); text(b, 12, 100, FT_TINY, C_MUT);
  text("OK = forward 420 ms, LEFT = reverse 420 ms,", 12, 196, FT_TINY, C_MUT);
  text("STOP = stop. Watch the chair / car.", 12, 210, FT_TINY, C_MUT);
  softKeys("send R", "next", "send F");
}
static void radioLive() {
  if (millis() - liveMs < 100) return; liveMs = millis();
  char b[40]; snprintf(b, sizeof(b), "%c", radioCmd);
  tft.fillRoundRect(12, 112, 110, 70, 8, radioCmd == 'F' ? C_GREEN : radioCmd == 'R' ? C_BLUE : C_PANEL);
  text(radioCmd == 'F' ? "FORWARD" : radioCmd == 'R' ? "REVERSE" : "STOP", 67, 144, FT_MD, C_WHITE, AL_C);
  text("sending now", 67, 166, FT_TINY, C_WHITE, AL_C);
  snprintf(b, sizeof(b), "%lu sent", (unsigned long)txOk); valueBox(140, 134, 170, b, FT_SM, C_FG);
  snprintf(b, sizeof(b), "%lu failed", (unsigned long)txFail); valueBox(140, 158, 170, b, FT_SM, txFail ? C_AMBER : C_MUT);
  snprintf(b, sizeof(b), "%lu pulses", (unsigned long)pulsesSent); valueBox(140, 180, 170, b, FT_TINY, C_MUT);
}
// ---- 8 REPORT
static void printReport() {
  Serial.println("\n======== NEUROX BOX TEST REPORT ========");
  for (uint8_t i = 0; i < PG_REPORT; ++i) Serial.printf("  %u %-9s %-5s %s\n", (unsigned)i + 1, PAGE_NAME[i], result[i] == R_PASS ? "PASS" : result[i] == R_FAIL ? "FAIL" : "-", note[i]);
  Serial.println("========================================");
}
static void drawReportPage() {
  uint8_t pass = 0;
  for (uint8_t i = 0; i < PG_REPORT; ++i) {
    const int16_t y = 26 + i * 25;
    const uint16_t c = result[i] == R_PASS ? C_GREEN : result[i] == R_FAIL ? C_RED : C_MUT;
    tft.fillRoundRect(10, y, 300, 23, 5, C_PANEL);
    text(PAGE_NAME[i], 20, y + 16, FT_MD, C_FG);
    text(result[i] == R_PASS ? "PASS" : result[i] == R_FAIL ? "FAIL" : "not done", 300, y + 16, FT_MD, c, AL_R);
    char nt[21]; strncpy(nt, note[i], 20); nt[20] = 0; text(nt, 118, y + 15, FT_TINY, C_MUT);
    if (result[i] == R_PASS) ++pass;
  }
  char b[32]; snprintf(b, sizeof(b), "%u / 7 passed", (unsigned)pass);
  text(b, 160, 216, FT_SM, pass == 7 ? C_GREEN : C_AMBER, AL_C);
  softKeys("", "start again", "print");
  printReport();
}

// ============================================================================
// navigation
// ============================================================================
static void enterPage(Page p) {
  if (page == PG_POWER && p != PG_POWER) {                 // leaving the power step: grade it
    if (!batOk()) setResult(PG_POWER, R_FAIL, batMv < 500 ? "battery reads 0 V" : "battery out of range");
    else if (!usbSeenHi) setResult(PG_POWER, R_FAIL, "battery ok, USB never seen");
    else setResult(PG_POWER, R_PASS, "battery + USB sense ok");
  }
  if (page == PG_RADIO && p != PG_RADIO) {
    const bool ok = radioReady && txOk > 10 && txFail * 20 < txOk + 1;
    setResult(PG_RADIO, ok ? R_PASS : R_FAIL, !radioReady ? "ESP-NOW did not start" : ok ? (pulsesSent ? "sending; check the chair moved" : "sending (no F/R tried)") : "too many failed sends");
    radioSend('S');
  }
  page = p; pageMs = millis(); liveMs = 0; lastDotX = -1; buzz(false); buzzing = false;
  setRot(rot); ts.setRotation(rot);                      // landscape, re-sent on every page
  tft.fillScreen(C_BG); header();
  switch (p) {
    case PG_SCREEN: drawScreenPage(); break;   case PG_BUTTONS: drawButtonsPage(); break;
    case PG_TOUCH: drawTouchPage(); break;     case PG_BUZZER: drawBuzzerPage(); break;
    case PG_MPU: drawMpuPage(); break;         case PG_POWER: drawPowerPage(); break;
    case PG_RADIO: drawRadioPage(); break;     case PG_REPORT: drawReportPage(); break;
    default: break;
  }
  if (p == PG_BUTTONS) { for (auto &b : BTNS) { b.seen = false; b.stuck = false; } for (uint8_t i = 0; i < 4; ++i) drawBtnTile(i); }
}
static void onKey(Key k) {
  if (k == K_STOP && page != PG_BUTTONS) { radioSend('S'); pulseEnd = millis(); }     // STOP always stops the chair
  switch (page) {
    case PG_SCREEN:
      if (k == K_OK) { setResult(PG_SCREEN, R_PASS, "looks right"); nextPage(); }
      else if (k == K_LEFT) { setResult(PG_SCREEN, R_FAIL, "user saw a problem"); nextPage(); }
      else if (k == K_NEW) { digitalWrite(PIN_TFT_LED, LOW); delay(700); digitalWrite(PIN_TFT_LED, HIGH); Serial.println("[SCREEN] backlight off 0.7 s, on again"); }
      break;
    case PG_TOUCH:
      if (k == K_LEFT) enterPage(PG_TOUCH);
      else if (k == K_NEW) { setResult(PG_TOUCH, R_FAIL, "skipped"); nextPage(); }
      else if (k == K_OK && calDone) nextPage();
      break;
    case PG_BUZZER:
      if (k == K_OK) { setResult(PG_BUZZER, R_PASS, "heard"); nextPage(); }
      else if (k == K_LEFT) { setResult(PG_BUZZER, R_FAIL, "silent"); nextPage(); }
      else if (k == K_NEW) { beep(800); buzzNextAt = millis() + 1600; }
      break;
    case PG_MPU:
      if (k == K_NEW) enterPage(PG_MPU); else if (k == K_OK) nextPage();
      break;
    case PG_POWER: if (k == K_OK) nextPage(); break;
    case PG_RADIO:
      if (k == K_OK) pulse('F'); else if (k == K_LEFT) pulse('R'); else if (k == K_NEW) nextPage();
      break;
    case PG_REPORT:
      if (k == K_NEW) { for (uint8_t i = 0; i < PG_N; ++i) { result[i] = R_NONE; note[i][0] = 0; } enterPage(PG_SCREEN); }
      else if (k == K_OK) printReport();
      break;
    default: break;
  }
}
static void buttonsService() {
  const uint32_t now = millis();
  for (uint8_t i = 0; i < 4; ++i) {
    Btn &b = BTNS[i];
    const bool lo = digitalRead(b.pin) == LOW;
    if (lo != b.down && now - b.t > 25) {
      b.down = lo; b.t = now;
      if (page == PG_BUTTONS) {
        if (lo) { b.seen = true; b.stuck = false; beep(40); Serial.printf("[BUTTONS] %s pressed (GPIO %d)\n", b.name, b.pin); }
        drawBtnTile(i);
        if (lo && b.key == K_STOP) { radioSend('S'); pulseEnd = now; }
      } else if (lo) { beep(15); onKey(b.key); }
    }
  }
  if (page == PG_BUTTONS) for (uint8_t i = 0; i < 4; ++i) {          // held LOW over 3 s = shorted to GND (or still held)
    Btn &b = BTNS[i]; const bool st = b.down && now - b.t > 3000;
    if (st != b.stuck) { b.stuck = st; drawBtnTile(i); if (st) Serial.printf("[BUTTONS] %s LOW for 3 s: shorted to GND?\n", b.name); }
  }
  if (page == PG_BUTTONS && result[PG_BUTTONS] != R_PASS && BTNS[0].seen && BTNS[1].seen && BTNS[2].seen && BTNS[3].seen) {
    setResult(PG_BUTTONS, R_PASS, "all 4 work"); tft.fillRect(10, 104, 205, 22, C_BG); text("all 4 work", 12, 120, FT_MD, C_GREEN); pageMs = now;
  }
  if (page == PG_BUTTONS && result[PG_BUTTONS] == R_PASS && now - pageMs > 1200 && !BTNS[0].down && !BTNS[1].down && !BTNS[2].down && !BTNS[3].down) nextPage();
}
static void serialService() {
  while (Serial.available()) {
    const char c = (char)Serial.read();
    if (c == 'n') { if (page == PG_BUTTONS && result[PG_BUTTONS] == R_NONE) setResult(PG_BUTTONS, R_FAIL, "skipped"); nextPage(); }
    else if (c == 'p') enterPage((Page)max<int>(page - 1, 0));
    else if (c >= '1' && c <= '8') enterPage((Page)(c - '1'));
    else if (c == 'r') printReport();
    else if (c == 'w') { rot = (uint8_t)((rot + 1) % 4); Serial.printf("[TFT] rotation %u %s\n", (unsigned)rot, (rot & 1) ? "(landscape)" : "(portrait: this layout needs 1 or 3)"); enterPage(page); }
  }
}

// ============================================================================
void setup() {
  Serial.begin(115200); delay(200);
  Serial.println("\nNeuroX Box Test v3.1  (n next, p previous, 1-8 jump, r report)");
  for (auto &b : BTNS) pinMode(b.pin, INPUT_PULLUP);
  pinMode(PIN_USB_SENSE, INPUT);
  pinMode(PIN_TFT_LED, OUTPUT); digitalWrite(PIN_TFT_LED, HIGH);
  analogReadResolution(12); analogSetPinAttenuation(PIN_VBAT, ADC_11db);
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
  ledcAttach(PIN_BUZZER, BUZZ_FREQ_HZ, 8);
#else
  ledcSetup(BUZZ_LEDC_CH, BUZZ_FREQ_HZ, 8); ledcAttachPin(PIN_BUZZER, BUZZ_LEDC_CH);
#endif
  buzz(false);
  pinMode(PIN_T_CS, OUTPUT); digitalWrite(PIN_T_CS, HIGH);      // keep the touch chip off the bus while the screen starts
  pinMode(PIN_TFT_CS, OUTPUT); digitalWrite(PIN_TFT_CS, HIGH);
  SPI.begin(PIN_TFT_SCK, PIN_TFT_MISO, PIN_TFT_MOSI, -1);
  screenBegin(); setRot(TFT_ROTATION);
  ts.begin(SPI); ts.setRotation(TOUCH_ROTATION);
  Wire.begin(PIN_SDA, PIN_SCL, 400000);
  radioInit();
  Serial.printf("[RADIO] ESP-NOW %s, box MAC %s\n", radioReady ? "ready" : "FAILED", WiFi.macAddress().c_str());
  setRot(rot);                                                       // once more, after everything else is up
  Serial.printf("[TFT] %s, rotation %u. Upside down? send w twice (or set TFT_ROTATION 1)\n", TFT_DRIVER == 7789 ? "ST7789" : "ILI9341", (unsigned)rot);
  enterPage(PG_SCREEN);
}
void loop() {
  serialService();
  buttonsService();
  radioService();
  if (buzzing && (int32_t)(millis() - buzzOffAt) >= 0) { buzz(false); buzzing = false; }
  switch (page) {
    case PG_TOUCH: touchLive(); break;   case PG_BUZZER: buzzerLive(); break;
    case PG_MPU: mpuLive(); break;       case PG_POWER: powerLive(); break;
    case PG_RADIO: radioLive(); break;   default: break;
  }
}
