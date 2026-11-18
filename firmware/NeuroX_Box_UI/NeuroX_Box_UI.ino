/*
 * ============================================================================
 *  NeuroX Box UI  v3.1   -  ESP32-S3 DevKitC-1 in the NeuroX box
 * ============================================================================
 *  The box receives both pill signals from the HALO (ESP32-C3) by ESP-NOW,
 *  detects blink / jaw clench / eyes firmly closed, runs the screen + buttons,
 *  and sends F / R / S to the wheelchair (same packet as NeuroX_Final_Sender,
 *  so RC_Car_LR_Receiver works unchanged).
 *
 *  FLOW (same as the firmware the box was built for)
 *    power on -> calibrate (relax, blink 5x, clench 5x, close eyes 3 s)
 *    -> LOCKED -> eyes firmly closed 3 s = unlock -> ACTIVE
 *    ACTIVE: clench = FORWARD 420 ms, blink = REVERSE 420 ms, 3 s idle = lock
 *    STOP button: stop + lock at any time.   NEW: calibrate again.
 *    OK: menu (Signals, Sensitivity, Radio, Power + tilt). Control pauses there.
 *
 *  TEST WITHOUT THE HALO: START_IN_SIM = 1 (default). The box makes its own
 *  test signals. Serial monitor 115200, send:
 *      b = blink   j = clench   e = eyes closed on/off   c = electrode off on/off
 *      l / n / o / s = LEFT / NEW / OK / STOP buttons
 *      m = switch source SIM <-> HALO   t = touch debug   ? = help
 *  The Radio page (menu) also switches SIM <-> HALO with OK.
 *
 *  NOTE: the gesture detector here is a simple, tunable one for testing the
 *  whole box. The full v13 engine from NeuroX_Final_Sender is not ported yet.
 *
 *  BOARD:  Tools > Board > "ESP32S3 Dev Module"   (Arduino-ESP32 core 3.x or 2.x)
 *          Serial monitor on the UART USB-C, or set "USB CDC On Boot: Enabled"
 *          to use the native USB-C.
 *  LIBRARIES (Library Manager):  Adafruit GFX Library, Adafruit ILI9341,
 *          Adafruit BusIO, XPT2046_Touchscreen (Paul Stoffregen)
 *
 *  PINS  (match the BOX S3 BOARD WIRING sheet)
 *    screen  SCK 12  MOSI 11  MISO 10  CS 9  DC 8  RESET 18  LED 14
 *    touch   T_CS 17  T_IRQ 16  (T_CLK / T_DIN / T_DO share SCK / MOSI / MISO)
 *    buttons OK 5  LEFT 6  NEW 7  STOP 15   (other leg to GND)
 *    buzzer 13     MPU-6050 SDA 41 SCL 42     battery sense 2   USB sense 21
 * ============================================================================
 */
#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <Preferences.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>

// =============================================================================
// 1. SETTINGS YOU MAY CHANGE
// =============================================================================
#define START_IN_SIM      1       // 1 = built-in test signals at power-on (no HALO needed)
#define TFT_ROTATION      3       // 1 or 3 = landscape. Picture upside down? use the other one.
#define TFT_SPI_HZ        40000000UL   // garbage on screen? try 27000000UL
#define TOUCH_ENABLED     1
#define TOUCH_SWAP_XY     0       // NeuroX_Box_Test prints this and the 4 TS_ values below
#define TOUCH_FLIP_X      0       // tap lands mirrored left/right? set 1
#define TOUCH_FLIP_Y      0       // tap lands mirrored up/down? set 1
#define BUZZER_ENABLED    1

static const uint16_t TS_MINX = 300, TS_MAXX = 3800, TS_MINY = 300, TS_MAXY = 3800;  // touch calibration (raw)
static const uint32_t AUTOLOCK_MS     = 3000;   // ACTIVE with no gesture this long -> LOCKED
static const uint32_t PULSE_MS        = 420;    // one FORWARD / REVERSE pulse
static const uint32_t REFRACTORY_MS   = 220;    // ignore a new gesture this soon after one
static const uint32_t UNLOCK_HOLD_MS  = 3000;   // eyes firmly closed this long = unlock
static const uint32_t MENU_TIMEOUT_MS = 20000;  // menu closes by itself (to LOCKED)
static const uint32_t HALO_TIMEOUT_MS = 500;    // no HALO packet this long = link lost

// =============================================================================
// 2. PINS
// =============================================================================
static const int8_t PIN_TFT_SCK = 12, PIN_TFT_MOSI = 11, PIN_TFT_MISO = 10;
static const int8_t PIN_TFT_CS = 9, PIN_TFT_DC = 8, PIN_TFT_RST = 18, PIN_TFT_LED = 14;
static const int8_t PIN_T_CS = 17, PIN_T_IRQ = 16;
static const int8_t PIN_BTN_OK = 5, PIN_BTN_LEFT = 6, PIN_BTN_NEW = 7, PIN_BTN_STOP = 15;
static const int8_t PIN_BUZZER = 13, PIN_SDA = 41, PIN_SCL = 42, PIN_VBAT = 2, PIN_USB_SENSE = 21;

// =============================================================================
// 3. RADIO  (ESP-NOW, channel 1)
// =============================================================================
static const uint8_t  ESPNOW_CHANNEL   = 1;
static const uint8_t  BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static const uint32_t HEARTBEAT_MS = 80, FAST_HEARTBEAT_MS = 25, FAST_HEARTBEAT_SPAN = 250;
static const uint8_t  RADIO_REPEATS = 2;
static const int8_t   RADIO_TX_POWER_QDBM = 60;        // 15 dBm

struct __attribute__((packed)) CarPacket {     // to the wheelchair - same as NeuroX_Final_Sender
  uint8_t magic0;   // 'N'
  uint8_t magic1;   // 'X'
  uint8_t cmd;      // 'F', 'R' or 'S'
  uint8_t seq;
};
static const uint8_t HALO_SAMPLES = 16;
struct __attribute__((packed)) HaloPacket {    // from the HALO C3 - keep in sync with NeuroX_HALO_C3.ino
  uint8_t  magic0;  // 'N'
  uint8_t  magic1;  // 'H'
  uint8_t  ver;     // 1
  uint8_t  flags;   // bit0 = a sample hit an ADC rail
  uint16_t seq;
  uint16_t battMv;  // HALO LiPo, mV
  uint16_t vert[HALO_SAMPLES];    // pill 2 forehead / nose (GPIO 1 on the C3), raw 12-bit, 256 Hz
  uint16_t horiz[HALO_SAMPLES];   // pill 1 temples        (GPIO 4 on the C3)
};

// =============================================================================
// 4. COLOURS + TYPES
// =============================================================================
// (a macro, not a function: the Arduino IDE puts its auto-prototypes above the first function)
#define RGB(r, g, b) ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))
static const uint16_t C_BG = RGB(10, 13, 19), C_PANEL = RGB(21, 26, 36), C_PANEL2 = RGB(29, 36, 49), C_LINE = RGB(42, 50, 66);
static const uint16_t C_FG = RGB(233, 237, 245), C_MUT = RGB(127, 138, 158), C_BLUE = RGB(47, 123, 255), C_TEAL = RGB(31, 198, 207);
static const uint16_t C_GREEN = RGB(43, 208, 123), C_AMBER = RGB(255, 176, 32), C_RED = RGB(255, 59, 59), C_VIOLET = RGB(168, 123, 255);
static const uint16_t C_WHITE = 0xFFFF, C_BLUEPALE = RGB(219, 230, 255);

enum Mode : uint8_t { M_BOOT, M_CAL, M_LOCKED, M_ACTIVE, M_MENU, M_PAGE };
enum Page : uint8_t { P_SIGNALS, P_SENS, P_RADIO, P_POWER, P_EXIT };
enum Key  : uint8_t { K_LEFT, K_NEW, K_OK, K_STOP };
enum Fnt  : uint8_t { FT_TINY, FT_SM, FT_MD, FT_LG, FT_XL };
enum Align : uint8_t { AL_L, AL_C, AL_R };
enum Ev   : uint8_t { EV_BLINK, EV_CLENCH, EV_CLENCH_END };

static const char *MENU_NAME[] = {"Signals", "Sensitivity", "Radio", "Power + tilt", "Exit"};
static const char *MENU_DESC[] = {"both pills live", "trigger level", "HALO + chair", "batteries, MPU", "back, locked"};
static const uint8_t MENU_N = 5;
static const float SENS_STEPS[] = {0.8f, 0.9f, 1.0f, 1.15f, 1.3f, 1.4f};
static const uint8_t SENS_N = 6;

// =============================================================================
// 5. STATE
// =============================================================================
struct Flash { const char *title; const char *sub; uint16_t col; uint32_t until; };
struct Ui {
  Mode mode = M_BOOT; Page page = P_SIGNALS; uint8_t menuSel = 0;
  uint32_t modeMs = 0, idleMs = 0, menuMs = 0;
  uint8_t calStep = 0, calCount = 0;              // 0 relax, 1 blink, 2 clench, 3 eyes
  bool calibrated = false, savedCal = false;
  char cmd = 'S'; uint32_t pulseUntil = 0; char lastCmd = 0; uint32_t lastCmdMs = 0;
  uint32_t refrUntil = 0;
  Flash flash = {nullptr, nullptr, 0, 0};
  bool sim = START_IN_SIM;
  uint8_t sensIdx = 2;
};
static Ui U;

struct Thr { float blink = 300, clench = 200, unlock = 60; };   // detector thresholds (raw counts)
static Thr T;                                                     // calibrated
static Thr TP;                                                    // provisional (during calibration)

// signals / detector
struct Det {
  bool init = false;
  float baseV = 2048, baseH = 2048, lpV = 0, prevV = 0, prevH = 0, envV = 0, envH = 0;
  // blink
  bool blCand = false, blWaitLow = false; uint16_t blUp = 0; float blPeak = 0;
  // clench
  bool clOn = false; uint16_t clUp = 0, clDown = 0; float clPeak = 0;
  // eyes firmly closed (forehead squeeze)
  uint16_t sqLow = 0; uint32_t sqHold = 0; float sqSum = 0; uint32_t sqN = 0;
  // clipping
  uint16_t railRun = 0, okRun = 0; bool clipped = false;
};
static Det D;
static const uint16_t DISP_N = 320;                 // display history at 128 Hz (2.5 s)
static int16_t dispV[DISP_N], dispH[DISP_N]; static uint16_t dispHead = 0; static uint8_t dispDiv = 0;
static float emgShown = 0;

// calibration measurements
static float calRestLp = 0, calRestEnv = 0, calRestEnvV = 0; static uint32_t calRestN = 0;
static float calPeaks[5]; static uint8_t calPeakN = 0;

// HALO receive ring (filled in the Wi-Fi callback)
static const uint16_t RING = 1024;
static uint16_t ringV[RING], ringH[RING];
static volatile uint16_t ringW = 0; static uint16_t ringR = 0;
static portMUX_TYPE rxMux = portMUX_INITIALIZER_UNLOCKED;
static volatile uint32_t haloLastMs = 0, haloPkts = 0, haloLost = 0;
static volatile uint16_t haloSeqLast = 0; static volatile bool haloSeqValid = false;
static volatile int16_t haloRssi = 0; static volatile uint16_t haloBattMv = 0;
static bool haloEverSeen = false;

// radio to the chair
static bool radioReady = false; static char radioCmd = 'S'; static uint8_t radioBurst = 0, radioSeq = 0;
static uint32_t radioChangeMs = 0, radioLastSendMs = 0, radioTxOk = 0, radioTxFail = 0;

// sim
static uint32_t simLastUs = 0; static float simAcc = 0; static uint32_t simT = 0;   // simT in samples
static uint32_t simBlinkT = 0xFFFFFFFF, simClenchT = 0xFFFFFFFF; static bool simEyes = false, simClip = false;
static float simEnv = 0; static uint32_t lcg = 12345;

// power, tilt
static float boxMv = 3900; static bool usbIn = false; static uint32_t pwrMs = 0;
static bool mpuOk = false; static float tiltPitch = 0, tiltRoll = 0; static uint32_t mpuMs = 0;

// hardware
static Adafruit_ILI9341 tft(&SPI, PIN_TFT_DC, PIN_TFT_CS, PIN_TFT_RST);
static XPT2046_Touchscreen ts(PIN_T_CS, PIN_T_IRQ);
static Preferences prefs;
static const int16_t SW = 320, SH = 240, STRIP_H = 40;
static GFXcanvas16 *cv = nullptr;
static int16_t OY = 0;                         // y of the strip being drawn
static uint8_t stripIdx = 0; static uint32_t frameMs = 0, nowMs = 0;
static const uint32_t FRAME_MS = 60;
static bool touchDebug = false;

// forward declarations
static void radioSend(char cmd);
static void beepPattern(const uint16_t *p, uint8_t n);
static void onEvent(Ev e, float val);
static void press(Key k);

// =============================================================================
// 6. BUZZER (2048 Hz, ON/OFF patterns, non-blocking)
// =============================================================================
static const uint32_t BUZZ_FREQ_HZ = 2048, BUZZ_DUTY = 128; static const uint8_t BUZZ_LEDC_CH = 2;
static uint16_t bzPat[16]; static uint8_t bzN = 0, bzI = 0; static uint32_t bzNext = 0; static bool bzOn = false;
static const uint16_t BZ_TICK[] = {18}, BZ_SHORT[] = {45}, BZ_STEP[] = {60, 80, 90};
static const uint16_t BZ_MELODY[] = {90, 40, 90, 40, 90, 40, 260}, BZ_LOCK[] = {200}, BZ_STOP[] = {80, 60, 80, 60, 80};
static const uint16_t BZ_PULSE_F[] = {70}, BZ_PULSE_R[] = {30, 40, 30};
static void buzzOut(bool on) {
  if (!BUZZER_ENABLED) return;
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
  ledcWrite(PIN_BUZZER, on ? BUZZ_DUTY : 0);
#else
  ledcWrite(BUZZ_LEDC_CH, on ? BUZZ_DUTY : 0);
#endif
  bzOn = on;
}
static void buzzInit() {
  if (!BUZZER_ENABLED) return;
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
  ledcAttach(PIN_BUZZER, BUZZ_FREQ_HZ, 8);
#else
  ledcSetup(BUZZ_LEDC_CH, BUZZ_FREQ_HZ, 8); ledcAttachPin(PIN_BUZZER, BUZZ_LEDC_CH);
#endif
  buzzOut(false);
}
static void beepPattern(const uint16_t *p, uint8_t n) {
  if (n > 16) n = 16;
  for (uint8_t i = 0; i < n; ++i) bzPat[i] = p[i];
  bzN = n; bzI = 0; buzzOut(true); bzNext = millis() + bzPat[0];
}
#define BEEP(arr) beepPattern(arr, sizeof(arr) / sizeof(arr[0]))
static void buzzService() {
  if (bzN == 0 || (int32_t)(millis() - bzNext) < 0) return;
  if (++bzI >= bzN) { bzN = 0; buzzOut(false); return; }
  buzzOut((bzI & 1) == 0); bzNext = millis() + bzPat[bzI];
}

// =============================================================================
// 7. RADIO
// =============================================================================
static void onHaloPacket(const uint8_t *data, int len, int rssi) {
  if (U.sim || len != (int)sizeof(HaloPacket)) return;
  const HaloPacket *p = (const HaloPacket *)data;
  if (p->magic0 != 'N' || p->magic1 != 'H' || p->ver != 1) return;
  portENTER_CRITICAL(&rxMux);
  if (haloSeqValid) { uint16_t gap = (uint16_t)(p->seq - haloSeqLast - 1); if (gap < 200) haloLost += gap; }
  haloSeqLast = p->seq; haloSeqValid = true; haloPkts = haloPkts + 1;
  uint16_t w = ringW;
  for (uint8_t i = 0; i < HALO_SAMPLES; ++i) { ringV[w] = p->vert[i]; ringH[w] = p->horiz[i]; w = (uint16_t)((w + 1) % RING); }
  ringW = w;
  haloLastMs = millis(); haloBattMv = p->battMv; if (rssi != 0) haloRssi = (int16_t)rssi;
  portEXIT_CRITICAL(&rxMux);
}
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
static void onRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  int rssi = (info && info->rx_ctrl) ? info->rx_ctrl->rssi : 0;
  onHaloPacket(data, len, rssi);
}
#else
static void onRecv(const uint8_t *mac, const uint8_t *data, int len) { (void)mac; onHaloPacket(data, len, 0); }
#endif

static void radioInit() {
  WiFi.mode(WIFI_STA); WiFi.disconnect();
  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_max_tx_power(RADIO_TX_POWER_QDBM);
  if (esp_now_init() != ESP_OK) { Serial.println("[RADIO] ERROR: esp_now_init failed"); return; }
  esp_now_register_recv_cb(onRecv);
  esp_now_peer_info_t peer; memset(&peer, 0, sizeof(peer));
  memcpy(peer.peer_addr, BROADCAST_MAC, 6); peer.channel = ESPNOW_CHANNEL; peer.ifidx = WIFI_IF_STA; peer.encrypt = false;
  if (esp_now_add_peer(&peer) != ESP_OK) { Serial.println("[RADIO] ERROR: could not add broadcast peer"); return; }
  radioReady = true;
  Serial.printf("[RADIO] ESP-NOW ch %u, box MAC %s\n", (unsigned)ESPNOW_CHANNEL, WiFi.macAddress().c_str());
  radioSend('S');
}
static void radioSend(char cmd) { radioCmd = cmd; radioBurst = (uint8_t)(1 + RADIO_REPEATS); radioChangeMs = millis(); }
static void radioService() {
  if (!radioReady) return;
  const uint32_t now = millis();
  const uint32_t period = (now - radioChangeMs < FAST_HEARTBEAT_SPAN) ? FAST_HEARTBEAT_MS : HEARTBEAT_MS;
  if (radioBurst == 0 && now - radioLastSendMs < period) return;
  CarPacket pkt = {'N', 'X', (uint8_t)radioCmd, radioSeq};
  if (esp_now_send(BROADCAST_MAC, (const uint8_t *)&pkt, sizeof(pkt)) == ESP_OK) {
    ++radioSeq; ++radioTxOk; if (radioBurst) --radioBurst; radioLastSendMs = now;
  } else ++radioTxFail;
}
static bool haloAlive() { return U.sim || (haloEverSeen && millis() - haloLastMs < HALO_TIMEOUT_MS); }

// =============================================================================
// 8. DETECTOR (256 Hz, one sample pair at a time)
//    lpV  = forehead channel, DC removed, low-passed  -> blinks
//    envV / envH = high-frequency (muscle) envelope    -> clench, eyes squeezed
// =============================================================================
static float sensK() { return SENS_STEPS[U.sensIdx]; }
static const Thr &thr() { return (U.mode == M_CAL) ? TP : T; }
static float thrBlink() { return thr().blink / (U.mode == M_CAL ? 1.0f : sensK()); }
static float thrClench() { return thr().clench / (U.mode == M_CAL ? 1.0f : sensK()); }
static float thrUnlock() { return thr().unlock / (U.mode == M_CAL ? 1.0f : sensK()); }

static void detStep(uint16_t rv, uint16_t rh) {
  // clipping (electrode off): 32 samples (125 ms) on a rail
  const bool rail = rv <= 16 || rv >= 4079 || rh <= 16 || rh >= 4079;
  if (rail) { D.okRun = 0; if (++D.railRun >= 32) D.clipped = true; }
  else { D.railRun = 0; if (D.clipped && ++D.okRun >= 256) D.clipped = false; }

  if (!D.init) { D.baseV = rv; D.baseH = rh; D.init = true; }
  const float xv = rv - D.baseV, xh = rh - D.baseH;
  D.baseV += xv * (1.0f / 512.0f); D.baseH += xh * (1.0f / 512.0f);          // ~2 s baseline
  D.lpV += (xv - D.lpV) * 0.25f;                                             // ~11 Hz low-pass
  const float dv = xv - D.prevV, dh = xh - D.prevH; D.prevV = xv; D.prevH = xh;
  D.envV += (fabsf(dv) - D.envV) * (1.0f / 24.0f); D.envH += (fabsf(dh) - D.envH) * (1.0f / 24.0f);
  const float emg = max(D.envV, D.envH);
  emgShown = emg;
  if (++dispDiv >= 2) { dispDiv = 0; dispV[dispHead] = (int16_t)constrain(D.lpV, -2000.0f, 2000.0f);
                        dispH[dispHead] = (int16_t)constrain(xh, -2000.0f, 2000.0f); dispHead = (uint16_t)((dispHead + 1) % DISP_N); }
  if (D.clipped || !haloAlive()) { D.blCand = false; D.clOn = false; D.sqHold = 0; return; }

  // calibration step 0: measure rest
  if (U.mode == M_CAL && U.calStep == 0) { calRestLp += fabsf(D.lpV); calRestEnv += emg; calRestEnvV += D.envV; ++calRestN; }

  const float bt = thrBlink(), ct = thrClench(), ut = thrUnlock();
  // ---- blink: up over the threshold and back down within 0.4 s, no muscle burst
  if (D.blWaitLow) { if (D.lpV < bt * 0.5f) D.blWaitLow = false; }
  else if (!D.blCand) { if (D.lpV > bt && emg < ct * 0.6f) { D.blCand = true; D.blUp = 0; D.blPeak = D.lpV; } }
  else {
    ++D.blUp; if (D.lpV > D.blPeak) D.blPeak = D.lpV;
    if (emg > ct * 0.6f) { D.blCand = false; D.blWaitLow = true; }
    else if (D.lpV < bt * 0.5f) { D.blCand = false; if (D.blUp <= 102) onEvent(EV_BLINK, D.blPeak); }
    else if (D.blUp > 102) { D.blCand = false; D.blWaitLow = true; }    // eyes stayed closed: not a blink
  }
  // ---- clench: muscle over the threshold for 78 ms; released after 50 ms under half
  if (!D.clOn) { if (emg > ct) { if (++D.clUp >= 20) { D.clOn = true; D.clDown = 0; D.clPeak = emg; onEvent(EV_CLENCH, emg); } } else D.clUp = 0; }
  else {
    if (emg > D.clPeak) D.clPeak = emg;
    if (emg < ct * 0.5f) { if (++D.clDown >= 13) { D.clOn = false; D.clUp = 0; onEvent(EV_CLENCH_END, D.clPeak); } } else D.clDown = 0;
  }
  // ---- eyes firmly closed: forehead muscle over the unlock level, dips < 150 ms forgiven
  if (D.envV > ut) { D.sqLow = 0; ++D.sqHold; D.sqSum += D.envV; ++D.sqN; }
  else if (++D.sqLow > 38) { D.sqHold = 0; D.sqSum = 0; D.sqN = 0; }
}
static uint32_t holdMs() { return D.sqHold * 1000UL / 256UL; }

// ---- test signals (SIM) ----
static float rnd() { lcg = lcg * 1664525UL + 1013904223UL; return (float)(lcg >> 8) / 16777216.0f - 0.5f; }   // -0.5 .. 0.5
static void simSample(uint16_t &rv, uint16_t &rh) {
  const float t = simT / 256.0f;
  float v = rnd() * 26 + sinf(t * 1.11f) * 18, h = rnd() * 20 + sinf(t * 0.77f + 1) * 14;
  const float db = (simT - simBlinkT) / 256.0f, dc = (simT - simClenchT) / 256.0f;
  if (simBlinkT != 0xFFFFFFFF && db >= 0 && db < 0.5f) v += 620 * expf(-((db - 0.14f) * (db - 0.14f)) / 0.0035f);
  float emg = 22;
  if (simClenchT != 0xFFFFFFFF && dc >= 0 && dc < 0.55f) emg = 820 * min(1.0f, dc * 12) * (1 - max(0.0f, dc - 0.4f) / 0.15f);
  if (simEyes) { v += 260; emg = max(emg, 240.0f); }
  simEnv += (emg - simEnv) * 0.08f;
  v += rnd() * simEnv * 0.9f; h += rnd() * simEnv * 1.2f;
  int iv = 2048 + (int)v, ih = 2048 + (int)h;
  if (simClip) { iv = 4095; ih = 0; }
  rv = (uint16_t)constrain(iv, 0, 4095); rh = (uint16_t)constrain(ih, 0, 4095);
  ++simT;
}
static void signalService() {
  if (U.sim) {
    const uint32_t us = micros(); simAcc += (us - simLastUs) * 0.000256f; simLastUs = us;
    if (simAcc > 64) simAcc = 64;
    while (simAcc >= 1) { uint16_t rv, rh; simSample(rv, rh); detStep(rv, rh); simAcc -= 1; }
  } else {
    simLastUs = micros();
    if (haloPkts) haloEverSeen = true;
    while (true) {
      uint16_t rv, rh;
      portENTER_CRITICAL(&rxMux);
      if (ringR == ringW) { portEXIT_CRITICAL(&rxMux); break; }
      rv = ringV[ringR]; rh = ringH[ringR]; ringR = (uint16_t)((ringR + 1) % RING);
      portEXIT_CRITICAL(&rxMux);
      detStep(rv, rh);
    }
  }
}

// =============================================================================
// 9. FLOW
// =============================================================================
static void setMode(Mode m) { U.mode = m; U.modeMs = millis(); U.idleMs = millis(); U.menuMs = millis(); D.sqHold = 0; U.flash.title = nullptr; }
static void flash(const char *t, const char *s, uint16_t c, uint32_t ms = 1300) { U.flash = {t, s, c, millis() + ms}; }
static void saveCal() {
  prefs.begin("nxbox", false);
  prefs.putFloat("bl", T.blink); prefs.putFloat("cl", T.clench); prefs.putFloat("ul", T.unlock); prefs.putBool("ok", true);
  prefs.end(); U.savedCal = true;
}
static void loadSettings() {
  prefs.begin("nxbox", true);
  U.savedCal = prefs.getBool("ok", false);
  if (U.savedCal) { T.blink = prefs.getFloat("bl", T.blink); T.clench = prefs.getFloat("cl", T.clench); T.unlock = prefs.getFloat("ul", T.unlock); }
  U.sensIdx = prefs.getUChar("sens", 2); if (U.sensIdx >= SENS_N) U.sensIdx = 2;
  prefs.end();
}
static void saveSens() { prefs.begin("nxbox", false); prefs.putUChar("sens", U.sensIdx); prefs.end(); }

static void startCal() {
  setMode(M_CAL); U.calStep = 0; U.calCount = 0; calPeakN = 0;
  calRestLp = calRestEnv = calRestEnvV = 0; calRestN = 0;
  TP.blink = 1e9f; TP.clench = 1e9f; TP.unlock = 1e9f;             // nothing triggers while measuring rest
  if (U.cmd != 'S') { U.cmd = 'S'; radioSend('S'); }
  BEEP(BZ_STEP); Serial.println("[CAL] relax, look ahead");
}
static float median5(float *a, uint8_t n) {
  float b[5]; for (uint8_t i = 0; i < n; ++i) b[i] = a[i];
  for (uint8_t i = 0; i < n; ++i) for (uint8_t j = i + 1; j < n; ++j) if (b[j] < b[i]) { float t = b[i]; b[i] = b[j]; b[j] = t; }
  return b[n / 2];
}
static void calService() {
  if (U.mode != M_CAL) return;
  if (U.calStep == 0 && millis() - U.modeMs > 2500 && calRestN > 256) {
    calRestLp /= calRestN; calRestEnv /= calRestN; calRestEnvV /= calRestN;
    TP.blink = max(8.0f * calRestLp, 80.0f); TP.clench = max(5.0f * calRestEnv, 40.0f); TP.unlock = 1e9f;
    U.calStep = 1; U.calCount = 0; calPeakN = 0; BEEP(BZ_STEP);
    Serial.printf("[CAL] rest |lp| %.1f emg %.1f -> blink 5x\n", calRestLp, calRestEnv);
  }
  if (U.calStep == 3) {
    if (D.sqHold * 1000UL / 256UL >= UNLOCK_HOLD_MS) {
      const float mean = D.sqN ? D.sqSum / D.sqN : TP.unlock;
      T.unlock = constrain(0.6f * mean, 3.0f * calRestEnvV, 0.6f * T.clench);
      saveCal(); U.calibrated = true; setMode(M_LOCKED); BEEP(BZ_MELODY);
      flash("CALIBRATED", "close eyes 3 s to unlock", C_GREEN, 1600);
      Serial.printf("[CAL] done: blink %.0f  clench %.0f  unlock %.0f\n", T.blink, T.clench, T.unlock);
    }
  }
}
static void calEvent(Ev e, float v) {
  if (U.calStep == 1 && e == EV_BLINK) {
    calPeaks[calPeakN++] = v; ++U.calCount; BEEP(BZ_SHORT);
    if (U.calCount >= 5) {
      T.blink = max(0.5f * median5(calPeaks, 5), 4.0f * calRestLp + 40.0f);
      U.calStep = 2; U.calCount = 0; calPeakN = 0; BEEP(BZ_STEP); Serial.printf("[CAL] blink level %.0f -> clench 5x\n", T.blink);
    }
  } else if (U.calStep == 2 && e == EV_CLENCH_END) {
    calPeaks[calPeakN++] = v; ++U.calCount; BEEP(BZ_SHORT);
    if (U.calCount >= 5) {
      T.clench = max(0.5f * median5(calPeaks, 5), 3.0f * calRestEnv);
      TP.blink = 1e9f; TP.clench = 1e9f;                                     // no blink / clench in step 3
      TP.unlock = max(3.0f * calRestEnvV, 0.2f * T.clench);
      U.calStep = 3; U.calCount = 0; D.sqHold = 0; D.sqSum = 0; D.sqN = 0; BEEP(BZ_STEP);
      Serial.printf("[CAL] clench level %.0f -> close eyes firmly 3 s\n", T.clench);
    }
  }
}
static void pulse(char c) {
  U.cmd = c; U.pulseUntil = millis() + PULSE_MS; U.lastCmd = c; U.lastCmdMs = millis(); U.idleMs = millis();
  radioSend(c); if (c == 'F') BEEP(BZ_PULSE_F); else BEEP(BZ_PULSE_R);
  Serial.printf("[CHAIR] %s\n", c == 'F' ? "FORWARD" : "REVERSE");
}
static void lockNow(const char *title, const char *sub, uint16_t col) {
  if (U.cmd != 'S') U.cmd = 'S';
  radioSend('S'); setMode(M_LOCKED); BEEP(BZ_LOCK);
  if (title) flash(title, sub, col);
}
static void onEvent(Ev e, float v) {
  if (U.mode == M_CAL) { if (U.calStep == 1 || U.calStep == 2) calEvent(e, v); return; }
  if (e == EV_CLENCH_END) return;
  if (U.mode != M_ACTIVE) return;
  if ((int32_t)(millis() - U.refrUntil) < 0 || (int32_t)(millis() - U.pulseUntil) < 0) return;
  U.refrUntil = millis() + PULSE_MS + REFRACTORY_MS;
  pulse(e == EV_CLENCH ? 'F' : 'R');
}
static void flowService() {
  const uint32_t now = millis();
  if (U.mode == M_BOOT) { if (now - U.modeMs > 2600) startCal(); return; }
  calService();
  if (U.mode == M_LOCKED && U.calibrated && haloAlive() && !D.clipped && holdMs() >= UNLOCK_HOLD_MS) {
    setMode(M_ACTIVE); BEEP(BZ_MELODY); radioSend('S'); Serial.println("[STATE] unlocked");
  }
  if (U.mode == M_ACTIVE) {
    if (U.cmd != 'S' && (int32_t)(now - U.pulseUntil) >= 0) { U.cmd = 'S'; radioSend('S'); U.idleMs = now; }
    if (!haloAlive()) lockNow("HALO LINK LOST", "chair stopped", C_RED);
    else if (D.clipped) lockNow("CLIPPED", "re-gel the pads", C_AMBER);
    else if (U.cmd == 'S' && now - U.idleMs >= AUTOLOCK_MS) lockNow("AUTO-LOCKED", "close eyes 3 s to unlock", C_AMBER);
  }
  if ((U.mode == M_MENU || U.mode == M_PAGE) && now - U.menuMs > MENU_TIMEOUT_MS) setMode(M_LOCKED);
}
static void press(Key k) {
  BEEP(BZ_TICK);
  if (k == K_STOP) {                                 // works in every state
    U.cmd = 'S'; radioSend('S'); Serial.println("[STOP] stop + lock");
    if (U.mode != M_BOOT) { setMode(M_LOCKED); BEEP(BZ_STOP); flash("STOPPED", "locked, chair stopped", C_RED); }
    return;
  }
  if (U.mode == M_BOOT) return;
  if (k == K_NEW) { startCal(); return; }
  U.menuMs = millis();
  switch (U.mode) {
    case M_CAL:
      if (k == K_OK && U.savedCal) { U.calibrated = true; setMode(M_LOCKED); BEEP(BZ_STEP); flash("SAVED LEVELS", "close eyes 3 s to unlock", C_GREEN); }
      break;
    case M_LOCKED: case M_ACTIVE:
      if (k == K_OK) { if (U.mode == M_ACTIVE) { U.cmd = 'S'; radioSend('S'); } setMode(M_MENU); U.menuSel = 0; }
      break;
    case M_MENU:
      if (k == K_LEFT) U.menuSel = (uint8_t)((U.menuSel + 1) % MENU_N);
      if (k == K_OK) { if (U.menuSel == P_EXIT) setMode(M_LOCKED); else { U.page = (Page)U.menuSel; setMode(M_PAGE); } }
      break;
    case M_PAGE:
      if (k == K_LEFT) setMode(M_MENU);
      else if (k == K_OK) {
        if (U.page == P_SENS) { U.sensIdx = (uint8_t)((U.sensIdx + 1) % SENS_N); saveSens(); }
        else if (U.page == P_RADIO) { U.sim = !U.sim; D.init = false; haloSeqValid = false; Serial.printf("[SOURCE] %s\n", U.sim ? "SIM" : "HALO"); }
        else setMode(M_MENU);
      }
      break;
    default: break;
  }
}

// =============================================================================
// 10. BUTTONS, TOUCH, SERIAL
// =============================================================================
struct Btn { int8_t pin; Key key; bool down; uint32_t t; };
static Btn BTNS[] = {{PIN_BTN_LEFT, K_LEFT, false, 0}, {PIN_BTN_NEW, K_NEW, false, 0}, {PIN_BTN_OK, K_OK, false, 0}, {PIN_BTN_STOP, K_STOP, false, 0}};
static void buttonsService() {
  const uint32_t now = millis();
  for (auto &b : BTNS) {
    const bool lo = digitalRead(b.pin) == LOW;
    if (lo && !b.down && now - b.t > (b.key == K_STOP ? 5UL : 25UL)) { b.down = true; b.t = now; press(b.key); }   // STOP acts at once
    else if (!lo && b.down && now - b.t > 25) { b.down = false; b.t = now; }
  }
}
static const int16_t SOFT_X[3] = {38, 170, 300};          // above LEFT / NEW / OK on the lid
static void handleTap(int16_t x, int16_t y) {
  if (U.mode == M_MENU && y >= 50 && y < 50 + MENU_N * 33) { U.menuSel = (uint8_t)((y - 50) / 33); press(K_OK); return; }
  if (y > 214) { int best = 0; for (int i = 1; i < 3; ++i) if (abs(SOFT_X[i] - x) < abs(SOFT_X[best] - x)) best = i;
                 press(best == 0 ? K_LEFT : best == 1 ? K_NEW : K_OK); }
}
static bool tWas = false; static uint32_t tMs = 0;
static void touchService() {
  if (!TOUCH_ENABLED || stripIdx != 0 || millis() - tMs < 30) return;    // never in the middle of a frame
  tMs = millis();
  const bool t = ts.touched();
  if (t && !tWas) {
    TS_Point p = ts.getPoint();
    if (TOUCH_SWAP_XY) { const int16_t t2 = p.x; p.x = p.y; p.y = t2; }
    int16_t x = (int16_t)map(p.x, TS_MINX, TS_MAXX, 0, SW), y = (int16_t)map(p.y, TS_MINY, TS_MAXY, 0, SH);
    if (TOUCH_FLIP_X) x = SW - x; if (TOUCH_FLIP_Y) y = SH - y;
    x = constrain(x, 0, SW - 1); y = constrain(y, 0, SH - 1);
    if (touchDebug) Serial.printf("[TOUCH] raw %d,%d z %d -> %d,%d\n", p.x, p.y, p.z, x, y);
    handleTap(x, y);
  }
  tWas = t;
}
static void help() {
  Serial.println(F("keys: b blink  j clench  e eyes on/off  c electrode off on/off  (SIM)\n"
                   "      l LEFT  n NEW  o OK  s STOP   m SIM<->HALO   t touch debug   ? help"));
}
static void serialService() {
  while (Serial.available()) {
    const char c = (char)Serial.read();
    switch (c) {
      case 'b': simBlinkT = simT; break;
      case 'j': simClenchT = simT; break;
      case 'e': simEyes = !simEyes; Serial.printf("[SIM] eyes %s\n", simEyes ? "CLOSED" : "open"); break;
      case 'c': simClip = !simClip; Serial.printf("[SIM] electrode %s\n", simClip ? "OFF" : "on"); break;
      case 'l': press(K_LEFT); break;  case 'n': press(K_NEW); break;
      case 'o': press(K_OK); break;    case 's': press(K_STOP); break;
      case 'm': U.sim = !U.sim; D.init = false; haloSeqValid = false; Serial.printf("[SOURCE] %s\n", U.sim ? "SIM" : "HALO"); break;
      case 't': touchDebug = !touchDebug; break;
      case '?': help(); break;
      default: break;
    }
  }
}

// =============================================================================
// 11. POWER + TILT
// =============================================================================
static uint8_t lipoPct(float mv) {
  static const float V[] = {3300, 3500, 3600, 3700, 3800, 3900, 4000, 4100, 4200};
  static const float P[] = {0, 5, 15, 30, 50, 65, 80, 90, 100};
  if (mv <= V[0]) return 0; if (mv >= V[8]) return 100;
  for (int i = 1; i < 9; ++i) if (mv < V[i]) return (uint8_t)(P[i - 1] + (P[i] - P[i - 1]) * (mv - V[i - 1]) / (V[i] - V[i - 1]));
  return 100;
}
static void mpuInit() {
  Wire.begin(PIN_SDA, PIN_SCL, 400000);
  Wire.beginTransmission(0x68); Wire.write(0x6B); Wire.write(0x00);       // wake up
  mpuOk = Wire.endTransmission() == 0;
  Serial.printf("[MPU] %s\n", mpuOk ? "MPU-6050 found at 0x68" : "not found (check SDA 41 / SCL 42 / 3V3)");
}
static void powerService() {
  const uint32_t now = millis();
  if (now - pwrMs >= 500) {
    pwrMs = now;
    const float mv = analogReadMilliVolts(PIN_VBAT) * 2.0f;                 // 100k / 100k divider
    boxMv += (mv - boxMv) * 0.2f;
    usbIn = digitalRead(PIN_USB_SENSE) == HIGH;
  }
  if (mpuOk && now - mpuMs >= 100) {
    mpuMs = now;
    Wire.beginTransmission(0x68); Wire.write(0x3B);
    if (Wire.endTransmission(false) == 0 && Wire.requestFrom(0x68, 6) == 6) {
      const int16_t ax = (int16_t)((Wire.read() << 8) | Wire.read());
      const int16_t ay = (int16_t)((Wire.read() << 8) | Wire.read());
      const int16_t az = (int16_t)((Wire.read() << 8) | Wire.read());
      const float p = atan2f((float)ax, sqrtf((float)ay * ay + (float)az * az)) * 57.2958f;
      const float r = atan2f((float)ay, (float)az) * 57.2958f;
      tiltPitch += (p - tiltPitch) * 0.3f; tiltRoll += (r - tiltRoll) * 0.3f;
    }
  }
}

// =============================================================================
// 12. DRAWING  (all in screen pixels, 320 x 240; drawn in 40-px strips)
// =============================================================================
static inline bool vis(int16_t y, int16_t h) { return y < OY + STRIP_H && y + h > OY; }
static void fillR(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c) { if (vis(y, h)) cv->fillRect(x, y - OY, w, h, c); }
static void fillRR(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t c) { if (w > 0 && vis(y, h)) cv->fillRoundRect(x, y - OY, w, h, min<int16_t>(r, min(w, h) / 2), c); }
static void drawRR(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t c) { if (vis(y, h)) cv->drawRoundRect(x, y - OY, w, h, r, c); }
static void fillC(int16_t x, int16_t y, int16_t r, uint16_t c) { if (vis(y - r, 2 * r + 1)) cv->fillCircle(x, y - OY, r, c); }
static void drawC(int16_t x, int16_t y, int16_t r, uint16_t c) { if (vis(y - r, 2 * r + 1)) cv->drawCircle(x, y - OY, r, c); }
static void lineT(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t c, uint8_t th = 1) {
  if (!vis(min(y0, y1) - th, abs(y1 - y0) + 2 * th + 1)) return;
  for (int8_t k = -(int8_t)(th / 2); k <= (int8_t)((th - 1) / 2); ++k) {
    if (abs(x1 - x0) >= abs(y1 - y0)) cv->drawLine(x0, y0 + k - OY, x1, y1 + k - OY, c); else cv->drawLine(x0 + k, y0 - OY, x1 + k, y1 - OY, c);
  }
}
static void triF(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t c) {
  int16_t a = min(y0, min(y1, y2)), b = max(y0, max(y1, y2)); if (vis(a, b - a + 1)) cv->fillTriangle(x0, y0 - OY, x1, y1 - OY, x2, y2 - OY, c);
}
static const GFXfont *fontOf(Fnt f) {
  switch (f) { case FT_SM: return &FreeSans9pt7b; case FT_MD: return &FreeSansBold9pt7b; case FT_LG: return &FreeSansBold12pt7b; case FT_XL: return &FreeSansBold18pt7b; default: return nullptr; }
}
static const int8_t ASC[] = {7, 13, 13, 17, 25}, DESC[] = {1, 4, 4, 5, 7};
static int16_t textW(const char *s, Fnt f) {
  if (f == FT_TINY) return (int16_t)(strlen(s) * 6);
  cv->setFont(fontOf(f)); int16_t x1, y1; uint16_t w, h; cv->getTextBounds(s, 0, 0, &x1, &y1, &w, &h); return (int16_t)(x1 + w);
}
// y = baseline
static void text(const char *s, int16_t x, int16_t y, Fnt f, uint16_t col, Align a = AL_L) {
  if (!vis(y - ASC[f] - 2, ASC[f] + DESC[f] + 4)) return;
  const int16_t w = (a == AL_L) ? 0 : textW(s, f);
  const int16_t xx = (a == AL_L) ? x : (a == AL_C) ? x - w / 2 : x - w;
  cv->setFont(fontOf(f)); cv->setTextSize(1); cv->setTextColor(col); cv->setTextWrap(false);
  cv->setCursor(xx, (f == FT_TINY ? y - 7 : y) - OY); cv->print(s);
}
static void ring(int16_t cx, int16_t cy, int16_t r, float frac, uint16_t col, uint8_t w) {
  if (!vis(cy - r - w, 2 * (r + w) + 1)) return;
  frac = constrain(frac, 0.0f, 1.0f);
  for (int a = 0; a < 360; a += 6) {
    const float rad = (a - 90) * 0.0174533f;
    fillC((int16_t)(cx + cosf(rad) * r), (int16_t)(cy + sinf(rad) * r), w / 2, (a < frac * 360) ? col : C_LINE);
  }
}
static void lockIcon(int16_t x, int16_t y, int16_t s, uint16_t col, bool open) {
  const int16_t r = s * 28 / 100, cx = open ? x + s * 66 / 100 : x + s / 2, by = y + s * 40 / 100, th = max<int16_t>(2, s / 8);
  if (vis(y - 2, s + 4)) for (int16_t k = 0; k < th; ++k) cv->drawCircleHelper(cx, by - OY, r - k, 1 | 2, col);
  fillR(cx - r, by, th, s * 10 / 100, col); fillR(cx + r - th + 1, by - (open ? s * 15 / 100 : 0), th, s * 10 / 100 + (open ? s * 15 / 100 : 0), col);
  fillRR(x + s * 12 / 100, by + s * 8 / 100, s * 76 / 100, s * 54 / 100, s / 10, col);
  fillR(x + s / 2 - 1, by + s * 24 / 100, 3, s * 20 / 100, C_BG);
}
static void battIcon(int16_t x, int16_t y, uint8_t pct, bool chg, const char *label) {
  const uint16_t col = pct < 15 ? C_RED : pct < 30 ? C_AMBER : C_FG;
  text(label, x - 2, y + 8, FT_TINY, C_MUT, AL_R);
  drawRR(x, y, 19, 10, 2, col); fillR(x + 19, y + 3, 2, 4, col); fillR(x + 2, y + 2, max(1, 15 * pct / 100), 6, col);
  if (chg) { triF(x + 11, y - 1, x + 6, y + 6, x + 10, y + 6, C_AMBER); triF(x + 8, y + 11, x + 13, y + 4, x + 9, y + 4, C_AMBER); }
}
static uint8_t haloPct() { return U.sim ? 64 : lipoPct(haloBattMv); }
static void statusBar() {
  fillR(0, 0, SW, 20, C_PANEL);
  text("NEUROX", 6, 15, FT_MD, C_BLUE);
  const bool alive = haloAlive();
  const int16_t rs = U.sim ? -54 : haloRssi;
  const uint8_t q = !alive ? 0 : (rs > -60 ? 3 : rs > -72 ? 2 : 1);
  text("HALO", 88, 14, FT_TINY, alive ? C_FG : C_RED);
  for (uint8_t i = 0; i < 3; ++i) fillR(114 + i * 4, 12 - i * 3, 3, 3 + i * 3, i < q ? C_TEAL : C_LINE);
  text("CHAIR", 132, 14, FT_TINY, C_FG); fillC(166, 10, 3, radioReady ? (alive ? C_GREEN : C_AMBER) : C_RED);
  battIcon(190, 5, haloPct(), false, "H"); battIcon(242, 5, lipoPct(boxMv), usbIn, "BOX");
  if (U.sim) { fillRR(268, 4, 24, 12, 3, C_AMBER); text("SIM", 280, 14, FT_TINY, C_BG, AL_C); }
  lockIcon(299, 3, 15, U.mode == M_ACTIVE ? C_GREEN : C_AMBER, U.mode == M_ACTIVE);
}
static void softKeys(const char *a, const char *b, const char *c) {
  fillR(0, 222, SW, 18, C_PANEL);
  if (a && *a) text(a, SOFT_X[0], 236, FT_SM, C_BLUE, AL_C);
  if (b && *b) text(b, SOFT_X[1], 236, FT_SM, C_FG, AL_C);
  if (c && *c) text(c, SW - 6, 236, FT_SM, C_BLUE, AL_R);
}
static void trace(const int16_t *buf, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t col, float scale, const char *label) {
  if (!vis(y, h)) return;
  fillRR(x, y, w, h, 4, C_PANEL); fillR(x + 2, y + h / 2, w - 4, 1, C_LINE);
  int16_t px = 0, py = 0;
  for (int16_t i = 0; i < w - 4; ++i) {
    const uint16_t k = (uint16_t)((dispHead + DISP_N - (w - 4) + i) % DISP_N);
    int16_t yy = (int16_t)(y + h / 2 - buf[k] * scale); yy = constrain(yy, y + 2, y + h - 3);
    if (i) lineT(x + 2 + i - 1, py, x + 2 + i, yy, col); px = x + 2 + i; py = yy;
  }
  (void)px;
  text(label, x + 6, y + 11, FT_TINY, C_MUT);
}
static void emgBar(int16_t x, int16_t y, int16_t w, int16_t h) {
  const float ct = thrClench(); const float v = min(1.0f, emgShown / max(1.0f, ct * 2.0f));
  fillRR(x, y, w, h, 3, C_PANEL);
  fillRR(x + 1, y + 1, (int16_t)((w - 2) * v), h - 2, 2, emgShown > ct ? C_GREEN : C_TEAL);
  fillR(x + (w - 2) / 2, y - 2, 2, h + 4, C_AMBER);                 // threshold mark at the middle
  text("EMG", x - 4, y + h - 1, FT_TINY, C_MUT, AL_R);
}
static void dots(int16_t cx, int16_t y, uint8_t n, uint8_t on) { for (uint8_t i = 0; i < n; ++i) fillC(cx - 48 + i * 24, y, 7, i < on ? C_GREEN : C_LINE); }

static void drawBoot() {
  const uint32_t t = nowMs - U.modeMs; const float p = min(1.0f, t / 2400.0f);
  if (vis(60, 80)) for (int a = 0; a < (int)(p * 315); a += 5) { const float r = (a - 140) * 0.0174533f; fillC((int16_t)(160 + cosf(r) * 30), (int16_t)(96 + sinf(r) * 30), 2, C_BLUE); }
  lineT(147, 110, 147, 82, C_BLUE, 4); lineT(147, 82, 173, 110, C_BLUE, 4); lineT(173, 110, 173, 82, C_BLUE, 4);
  fillC(183, 73, 3, C_RED);
  text("NEUROX", 160, 156, FT_XL, C_FG, AL_C);
  const char *s = t < 1300 ? "looking for the HALO..." : U.sim ? "test signals (SIM)" : haloAlive() ? "HALO linked" : "HALO not found yet";
  text(s, 160, 182, FT_SM, t < 1300 ? C_MUT : (U.sim || haloAlive()) ? C_TEAL : C_AMBER, AL_C);
  text("box v3.1  ESP32-S3", 160, 210, FT_TINY, C_MUT, AL_C);
}
static void drawCal() {
  statusBar();
  char b[24]; snprintf(b, sizeof(b), "CALIBRATING %u / 3", (unsigned)max<uint8_t>(1, U.calStep));
  text(U.calStep == 0 ? "GET READY" : b, 12, 40, FT_MD, C_AMBER);
  for (uint8_t i = 1; i <= 3; ++i) fillRR(190 + (i - 1) * 42, 32, 38, 5, 2, i < U.calStep ? C_GREEN : i == U.calStep ? C_AMBER : C_LINE);
  static const char *T1[] = {"RELAX", "BLINK", "CLENCH", "CLOSE EYES"};
  static const char *T2[] = {"look ahead, don't move", "5 times, firmly", "your jaw 5 times", "firmly, hold 3 s"};
  text(T1[U.calStep], 160, 80, FT_XL, C_FG, AL_C);
  text(T2[U.calStep], 160, 102, FT_SM, C_MUT, AL_C);
  if (U.calStep == 1 || U.calStep == 2) dots(160, 120, 5, U.calCount);
  else if (U.calStep == 3) { ring(160, 122, 14, holdMs() / (float)UNLOCK_HOLD_MS, C_GREEN, 4); char h[8]; snprintf(h, sizeof(h), "%.1f", holdMs() / 1000.0f); text(h, 160, 126, FT_TINY, C_FG, AL_C); }
  else { const float p = min(1.0f, (nowMs - U.modeMs) / 2500.0f); fillRR(80, 116, 160, 6, 3, C_PANEL); fillRR(80, 116, (int16_t)(160 * p), 6, 3, C_AMBER); }
  const bool temples = U.calStep == 2;
  trace(temples ? dispH : dispV, 12, 144, 296, 72, temples ? C_VIOLET : C_TEAL, temples ? 0.045f : 0.05f, temples ? "TEMPLES  (pill 1)" : "FOREHEAD  (pill 2)");
  softKeys("", "restart", U.savedCal ? "use saved" : "");
}
static void drawLocked() {
  statusBar();
  lockIcon(140, 40, 40, C_AMBER, false);
  text("LOCKED", 160, 116, FT_XL, C_AMBER, AL_C);
  if (!U.calibrated) text("not calibrated: press NEW", 160, 140, FT_SM, C_MUT, AL_C);
  else {
    text("close eyes firmly 3 s", 160, 140, FT_SM, C_MUT, AL_C);
    fillRR(60, 154, 200, 10, 5, C_PANEL); fillRR(60, 154, (int16_t)(200 * min(1.0f, holdMs() / (float)UNLOCK_HOLD_MS)), 10, 5, C_GREEN);
    char h[16]; if (holdMs() > 0) snprintf(h, sizeof(h), "%.1f s", holdMs() / 1000.0f); else snprintf(h, sizeof(h), "eyes open");
    text(h, 160, 184, FT_TINY, holdMs() ? C_GREEN : C_MUT, AL_C);
  }
  text("chair stopped", 160, 206, FT_TINY, C_MUT, AL_C);
  softKeys("", "recalibrate", "menu");
}
static void drawActive() {
  statusBar();
  const bool live = (int32_t)(nowMs - U.pulseUntil) < 0; const char c = live ? U.cmd : 'S';
  fillRR(10, 28, 190, 98, 8, c == 'F' ? C_GREEN : c == 'R' ? C_BLUE : C_PANEL2);
  text(c == 'F' ? "FORWARD" : c == 'R' ? "REVERSE" : "READY", 105, 78, FT_LG, c == 'S' ? C_FG : C_WHITE, AL_C);
  if (c == 'F') triF(105, 90, 93, 110, 117, 110, C_WHITE);
  else if (c == 'R') triF(105, 110, 93, 90, 117, 90, C_WHITE);
  else { text("clench = forward", 105, 100, FT_SM, C_MUT, AL_C); text("blink = reverse", 105, 116, FT_SM, C_MUT, AL_C); }
  fillRR(208, 28, 102, 46, 8, C_PANEL); text("LAST", 216, 42, FT_TINY, C_MUT);
  text(U.lastCmd ? (U.lastCmd == 'F' ? "FORWARD" : "REVERSE") : "-", 216, 64, FT_MD, C_FG);
  fillRR(208, 80, 102, 46, 8, C_PANEL); text("AUTO-LOCK", 216, 94, FT_TINY, C_MUT);
  const uint32_t idle = (U.cmd == 'S') ? nowMs - U.idleMs : 0, left = idle >= AUTOLOCK_MS ? 0 : AUTOLOCK_MS - idle;
  char b[8]; snprintf(b, sizeof(b), "%.1f", left / 1000.0f); text(b, 216, 118, FT_MD, C_FG);
  ring(288, 106, 11, left / (float)AUTOLOCK_MS, C_AMBER, 3);
  trace(dispV, 10, 134, 300, 40, C_TEAL, 0.03f, "FOREHEAD");
  trace(dispH, 10, 178, 230, 38, C_VIOLET, 0.025f, "TEMPLES");
  emgBar(270, 199, 40, 10);
  softKeys("", "recalibrate", "menu");
}
static void drawMenu() {
  statusBar();
  text("MENU", 12, 42, FT_MD, C_FG); text("control paused", 308, 42, FT_SM, C_AMBER, AL_R);
  for (uint8_t i = 0; i < MENU_N; ++i) {
    const int16_t y = 50 + i * 33; const bool on = i == U.menuSel;
    fillRR(10, y, 300, 29, 6, on ? C_BLUE : C_PANEL);
    text(MENU_NAME[i], 22, y + 20, FT_MD, on ? C_WHITE : C_FG); text(MENU_DESC[i], 298, y + 20, FT_SM, on ? C_BLUEPALE : C_MUT, AL_R);
  }
  softKeys("next", "recalibrate", "open");
}
static void rowKV(int16_t i, const char *k, const char *v, uint16_t vc, int16_t w = 300) {
  const int16_t y = 50 + i * 32; fillRR(10, y, w, 28, 6, C_PANEL);
  text(k, 20, y + 19, FT_SM, C_MUT); text(v, 10 + w - 10, y + 19, FT_MD, vc, AL_R);
}
static void drawPage() {
  statusBar();
  text(MENU_NAME[U.page], 12, 42, FT_MD, C_FG);
  char b[40];
  if (U.page == P_SIGNALS) {
    text("256 Hz  12-bit", 308, 42, FT_TINY, C_MUT, AL_R);
    trace(dispV, 10, 50, 300, 70, C_TEAL, 0.045f, "PILL 2  FOREHEAD / NOSE");
    trace(dispH, 10, 126, 300, 62, C_VIOLET, 0.04f, "PILL 1  TEMPLES");
    emgBar(44, 198, 266, 12);
    softKeys("back", "recalibrate", "back");
  } else if (U.page == P_SENS) {
    text("how hard a clench must be", 160, 68, FT_SM, C_MUT, AL_C);
    snprintf(b, sizeof(b), "x%.2f", sensK()); text(b, 160, 118, FT_XL, C_FG, AL_C);
    for (uint8_t i = 0; i < SENS_N; ++i) fillRR(70 + i * 31, 136, 26, 8, 3, i <= U.sensIdx ? C_BLUE : C_LINE);
    text("harder", 70, 164, FT_TINY, C_MUT); text("more sensitive", 250, 164, FT_TINY, C_MUT, AL_R);
    emgBar(44, 184, 266, 12);
    softKeys("back", "recalibrate", "change");
  } else if (U.page == P_RADIO) {
    const bool alive = haloAlive();
    if (U.sim) snprintf(b, sizeof(b), "SIM"); else if (alive) snprintf(b, sizeof(b), "%d dBm", (int)haloRssi); else snprintf(b, sizeof(b), "lost");
    rowKV(0, "HALO (C3)", b, U.sim ? C_AMBER : alive ? C_TEAL : C_RED);
    const uint32_t pk = haloPkts, ls = haloLost;
    snprintf(b, sizeof(b), "%.1f%% lost", (pk + ls) ? 100.0f * ls / (pk + ls) : 0.0f); rowKV(1, "  packets", U.sim ? "-" : b, C_FG);
    snprintf(b, sizeof(b), "%lu ok  %lu fail", (unsigned long)radioTxOk, (unsigned long)radioTxFail); rowKV(2, "CHAIR", b, radioReady ? C_GREEN : C_RED);
    snprintf(b, sizeof(b), "%c", U.cmd); rowKV(3, "  sending now", b, C_FG);
    rowKV(4, "source", U.sim ? "SIM" : "HALO", C_FG);
    softKeys("back", "recalibrate", U.sim ? "use HALO" : "use SIM");
  } else if (U.page == P_POWER) {
    snprintf(b, sizeof(b), "%u%%  %.2f V", (unsigned)lipoPct(boxMv), boxMv / 1000.0f); rowKV(0, "BOX", b, C_FG, 186);
    if (U.sim) snprintf(b, sizeof(b), "64%% (SIM)"); else snprintf(b, sizeof(b), "%u%%  %.2f V", (unsigned)lipoPct(haloBattMv), haloBattMv / 1000.0f);
    rowKV(1, "HALO", b, C_FG, 186);
    rowKV(2, "USB", usbIn ? "charging" : "not in", usbIn ? C_AMBER : C_FG, 186);
    fillRR(204, 50, 106, 156, 6, C_PANEL); text("TILT  MPU-6050", 257, 64, FT_TINY, C_MUT, AL_C);
    drawC(257, 120, 38, C_LINE); drawC(257, 120, 12, C_LINE);
    if (mpuOk) {
      const int16_t bx = (int16_t)(257 + constrain(tiltRoll, -30.0f, 30.0f) * 1.1f), by = (int16_t)(120 + constrain(tiltPitch, -30.0f, 30.0f) * 1.1f);
      fillC(bx, by, 8, (fabsf(tiltPitch) > 15 || fabsf(tiltRoll) > 15) ? C_AMBER : C_GREEN);
      snprintf(b, sizeof(b), "%+.0f / %+.0f", tiltPitch, tiltRoll); text(b, 257, 184, FT_MD, C_FG, AL_C);
      text("pitch / roll", 257, 198, FT_TINY, C_MUT, AL_C);
    } else text("not found", 257, 184, FT_SM, C_AMBER, AL_C);
    softKeys("back", "recalibrate", "back");
  }
}
static void overlay(const char *t, const char *s, uint16_t col) {
  fillRR(26, 70, 268, 96, 10, C_PANEL2); drawRR(26, 70, 268, 96, 10, col); drawRR(27, 71, 266, 94, 9, col);
  text(t, 160, 110, FT_LG, col, AL_C); text(s, 160, 138, FT_SM, C_FG, AL_C);
}
static void drawScreen() {
  switch (U.mode) {
    case M_BOOT: drawBoot(); break;   case M_CAL: drawCal(); break;
    case M_LOCKED: drawLocked(); break; case M_ACTIVE: drawActive(); break;
    case M_MENU: drawMenu(); break;   case M_PAGE: drawPage(); break;
  }
  if (U.mode == M_BOOT) return;
  if (!haloAlive()) overlay("HALO LINK LOST", "chair stopped, check HALO", C_RED);
  else if (D.clipped) overlay("CLIPPED", "electrode off: re-gel pads", C_AMBER);
  else if (U.flash.title && (int32_t)(nowMs - U.flash.until) < 0) overlay(U.flash.title, U.flash.sub, U.flash.col);
}
static void renderService() {
  if (!cv) return;
  if (stripIdx == 0) { if (millis() - frameMs < FRAME_MS) return; frameMs = millis(); nowMs = frameMs; }
  OY = (int16_t)(stripIdx * STRIP_H);
  cv->fillScreen(C_BG);
  drawScreen();
  tft.startWrite(); tft.setAddrWindow(0, OY, SW, STRIP_H); tft.writePixels(cv->getBuffer(), (uint32_t)SW * STRIP_H, true, false); tft.endWrite();
  stripIdx = (uint8_t)((stripIdx + 1) % (SH / STRIP_H));
}

// =============================================================================
// 13. SETUP / LOOP
// =============================================================================
void setup() {
  Serial.begin(115200); delay(200);
  Serial.println("\nNeuroX Box UI v3.1");
  for (auto &b : BTNS) pinMode(b.pin, INPUT_PULLUP);
  pinMode(PIN_USB_SENSE, INPUT);
  pinMode(PIN_TFT_LED, OUTPUT); digitalWrite(PIN_TFT_LED, HIGH);
  analogReadResolution(12); analogSetPinAttenuation(PIN_VBAT, ADC_11db);
  buzzInit();
  SPI.begin(PIN_TFT_SCK, PIN_TFT_MISO, PIN_TFT_MOSI, -1);
  tft.begin(TFT_SPI_HZ); tft.setRotation(TFT_ROTATION); tft.fillScreen(C_BG);
  if (TOUCH_ENABLED) { ts.begin(SPI); ts.setRotation(TFT_ROTATION); }
  cv = new GFXcanvas16(SW, STRIP_H);
  if (!cv || !cv->getBuffer()) Serial.println("[TFT] ERROR: no memory for the draw buffer");
  mpuInit();
  loadSettings();
  radioInit();
  setMode(M_BOOT);
  simLastUs = micros();
  help();
}
void loop() {
  serialService();
  buttonsService();
  signalService();
  flowService();
  radioService();
  buzzService();
  powerService();
  touchService();
  renderService();
}
