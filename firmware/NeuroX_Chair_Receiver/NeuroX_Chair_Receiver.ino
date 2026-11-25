/*
 * ============================================================================
 *  NeuroX Chair Receiver  v1.0  -  ESP32 DevKit on the wheelchair
 * ============================================================================
 *  Listens for the NeuroX box (ESP-NOW broadcast, channel 1, packet {'N','X',cmd,seq})
 *  and drives 2 x BTS7960 (IBT-2). Each BTS7960 runs the 2 Johnson motors of one wheel,
 *  wired in parallel.
 *
 *    'F' = forward   'R' = reverse (slower)   'S' = stop
 *
 *  SAFETY (all in this file; the red E-STOP in the battery line is the real one)
 *    - Not ARMED at power-on: it ignores F / R until it has received an 'S'.
 *      If the box is already sending F when the chair is switched on, nothing moves.
 *    - Link lost (no packet for LINK_TIMEOUT_MS) -> stop, and disarm again.
 *    - Soft start / soft stop: speed ramps, the rider is not jerked.
 *    - Hard speed cap: MAX_DUTY_PCT. Reverse is capped lower.
 *    - Direction change always passes through zero and waits ZERO_DWELL_MS.
 *    - Never drives RPWM and LPWM of one driver at the same time.
 *    - Stopped = brake (both low sides on): the chair holds still on flat ground.
 *
 *  WIRING (ESP32 DevKit 38-pin)
 *    LEFT  BTS7960:  RPWM -> GPIO 25   LPWM -> GPIO 26   R_EN + L_EN -> GPIO 27
 *    RIGHT BTS7960:  RPWM -> GPIO 32   LPWM -> GPIO 33   R_EN + L_EN -> GPIO 13
 *    both BTS7960:   VCC -> ESP32 3V3   GND -> ESP32 GND   (R_IS / L_IS not used)
 *    ESP32 5V/VIN <- LM2596 OUT set to 5.0 V, LM2596 IN <- 12 V battery after the switch.
 *    One common GND: battery -, both BTS7960 B-/GND, LM2596 -, ESP32 GND.
 *
 *  SERIAL 115200 (bench test with the wheels OFF the ground, box switched off):
 *    t  = guided test: left fwd, left rev, right fwd, right rev, both fwd, both rev
 *    f / r / s = drive like the box would (needs 'a' first)     a = arm
 *    + / -     = speed cap up / down 5 %                         ? = status
 *    Serial commands stop by themselves after 2 s, so the chair can't run away.
 *
 *  BOARD: "ESP32 Dev Module". Arduino-ESP32 core 2.x or 3.x.
 * ============================================================================
 */
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif

// ---------------- settings ----------------
#define MAX_DUTY_PCT      60      // forward speed cap (% of 12 V). Start low, raise after testing.
#define REV_DUTY_PCT      40      // reverse speed cap
#define ACCEL_PCT_PER_S   70      // speed-up rate:  0 -> 60 % in ~0.9 s
#define DECEL_PCT_PER_S   180     // slow-down rate: 60 -> 0 % in ~0.3 s (STOP, link lost)
#define ZERO_DWELL_MS     200     // pause at zero before changing direction
#define LINK_TIMEOUT_MS   300     // box sends every 80 ms (25 ms after a change)
#define INVERT_LEFT       0       // a wheel turns the wrong way? set its INVERT to 1
#define INVERT_RIGHT      1       // right side is mirrored, so it usually needs 1
#define LEFT_TRIM_PCT     100     // chair drifts right? lower LEFT_TRIM a little (e.g. 95)
#define RIGHT_TRIM_PCT    100
#define PWM_HZ            20000   // above hearing, fine for the BTS7960
#define PWM_BITS          10

static const uint8_t L_RPWM = 25, L_LPWM = 26, L_EN = 27;
static const uint8_t R_RPWM = 32, R_LPWM = 33, R_EN = 13;
static const uint8_t ESPNOW_CHANNEL = 1;
static const uint32_t SERIAL_CMD_MS = 2000;   // serial f / r runs at most this long

struct __attribute__((packed)) CarPacket { uint8_t magic0, magic1, cmd, seq; };   // 'N','X', 'F'/'R'/'S', counter

// ---------------- radio state (written in the Wi-Fi task) ----------------
static portMUX_TYPE rxLock = portMUX_INITIALIZER_UNLOCKED;
static volatile char rxCmd = 'S';
static volatile uint32_t rxMs = 0, rxGood = 0, rxBad = 0;
static volatile bool rxEver = false;

static void handlePacket(const uint8_t *d, int len) {
  const uint32_t now = millis();
  bool ok = len == (int)sizeof(CarPacket);
  const CarPacket *p = (const CarPacket *)d;
  if (ok) ok = p->magic0 == 'N' && p->magic1 == 'X' && (p->cmd == 'F' || p->cmd == 'R' || p->cmd == 'S');
  portENTER_CRITICAL(&rxLock);
  if (ok) { rxCmd = (char)p->cmd; rxMs = now; rxEver = true; rxGood = rxGood + 1; } else rxBad = rxBad + 1;
  portEXIT_CRITICAL(&rxLock);
}
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
static void onRecv(const esp_now_recv_info_t *, const uint8_t *d, int len) { handlePacket(d, len); }
#else
static void onRecv(const uint8_t *, const uint8_t *d, int len) { handlePacket(d, len); }
#endif

// ---------------- PWM (core 2.x and 3.x) ----------------
static const uint32_t PWM_MAX = (1u << PWM_BITS) - 1;
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
static void pwmInit(uint8_t pin, uint8_t) { ledcAttach(pin, PWM_HZ, PWM_BITS); ledcWrite(pin, 0); }
static void pwmSet(uint8_t pin, uint8_t, uint32_t d) { ledcWrite(pin, d); }
#else
static void pwmInit(uint8_t pin, uint8_t ch) { ledcSetup(ch, PWM_HZ, PWM_BITS); ledcAttachPin(pin, ch); ledcWrite(ch, 0); }
static void pwmSet(uint8_t, uint8_t ch, uint32_t d) { ledcWrite(ch, d); }
#endif

struct Side { uint8_t rpwm, lpwm, en, chR, chL; bool invert; int trim; };
static const Side LEFT = {L_RPWM, L_LPWM, L_EN, 0, 1, INVERT_LEFT != 0, LEFT_TRIM_PCT};
static const Side RIGHT = {R_RPWM, R_LPWM, R_EN, 2, 3, INVERT_RIGHT != 0, RIGHT_TRIM_PCT};

// pct: -100..100, + = chair forward. 0 = brake (both low sides on, EN stays high).
static void drive(const Side &s, float pct) {
  pct = constrain(pct, -100.0f, 100.0f) * s.trim / 100.0f;
  if (s.invert) pct = -pct;
  const uint32_t duty = (uint32_t)(fabsf(pct) * PWM_MAX / 100.0f + 0.5f);
  if (pct > 0) { pwmSet(s.lpwm, s.chL, 0); pwmSet(s.rpwm, s.chR, duty); }      // always clear the other side first
  else if (pct < 0) { pwmSet(s.rpwm, s.chR, 0); pwmSet(s.lpwm, s.chL, duty); }
  else { pwmSet(s.rpwm, s.chR, 0); pwmSet(s.lpwm, s.chL, 0); }
}

// ---------------- motion state ----------------
static bool armed = false, linkUp = false;
static float speed = 0;            // applied, -100..100 (both wheels), before trim
static float capFwd = MAX_DUTY_PCT;
static uint32_t zeroSinceMs = 0, lastMs = 0, statMs = 0;
static int8_t lastSign = 0;
// bench test from Serial (wheels off the ground)
static char serCmd = 0; static uint32_t serMs = 0;
static int8_t testStep = -1; static uint32_t testMs = 0;
static float testL = 0, testR = 0;

static float targetFor(char c) {
  if (c == 'F') return capFwd;
  if (c == 'R') return -(float)min((int)capFwd, REV_DUTY_PCT);
  return 0;
}

// moves `speed` toward `target` with ramps and a stop at zero between directions
static void ramp(float target, float dt, uint32_t now) {
  const int8_t sTarget = target > 0 ? 1 : target < 0 ? -1 : 0;
  const int8_t sNow = speed > 0.01f ? 1 : speed < -0.01f ? -1 : 0;
  if (sNow != 0 && sTarget != sNow) target = 0;                       // must reach zero first
  if (sNow == 0 && sTarget != 0) {
    if (lastSign != 0 && sTarget != lastSign && now - zeroSinceMs < ZERO_DWELL_MS) target = 0;   // dwell at zero
  }
  const bool slowing = fabsf(target) < fabsf(speed) || (target * speed < 0);
  const float step = (slowing ? DECEL_PCT_PER_S : ACCEL_PCT_PER_S) * dt;
  if (speed < target) speed = min(speed + step, target); else if (speed > target) speed = max(speed - step, target);
  const int8_t sAfter = speed > 0.01f ? 1 : speed < -0.01f ? -1 : 0;
  if (sAfter == 0) { if (sNow != 0) { zeroSinceMs = now; lastSign = sNow; } speed = 0; }
}

static void printStatus(const char *why) {
  Serial.printf("[CHAIR] %s  armed=%d link=%d speed=%.0f%% cap=%.0f%% good=%lu bad=%lu\n", why, armed, linkUp, speed, capFwd,
                (unsigned long)rxGood, (unsigned long)rxBad);
}

static void benchTest(uint32_t now) {
  // each step 1.5 s at 30 %, then 1 s stopped; wheels must be OFF the ground
  static const char *NAME[] = {"LEFT forward", "LEFT reverse", "RIGHT forward", "RIGHT reverse", "BOTH forward", "BOTH reverse"};
  static const int8_t L[] = {1, -1, 0, 0, 1, -1}, R[] = {0, 0, 1, -1, 1, -1};
  const uint32_t t = now - testMs; const uint8_t i = (uint8_t)(t / 2500); const bool run = (t % 2500) < 1500;
  if (i >= 6) { testStep = -1; testL = testR = 0; Serial.println("[TEST] done. Each wheel should have pushed the chair FORWARD when it said forward."); return; }
  if (i != testStep) { testStep = (int8_t)i; Serial.printf("[TEST] %d/6  %s  (30 %%)\n", i + 1, NAME[i]); }
  testL = run ? 30.0f * L[i] : 0; testR = run ? 30.0f * R[i] : 0;
}

void setup() {
  for (uint8_t p : {L_RPWM, L_LPWM, R_RPWM, R_LPWM, L_EN, R_EN}) { digitalWrite(p, LOW); pinMode(p, OUTPUT); }
  Serial.begin(115200); delay(200);
  Serial.println("\nNeuroX Chair Receiver v1.0  (2 x BTS7960, 2 motors per wheel)");
  pwmInit(L_RPWM, 0); pwmInit(L_LPWM, 1); pwmInit(R_RPWM, 2); pwmInit(R_LPWM, 3);
  drive(LEFT, 0); drive(RIGHT, 0);
  digitalWrite(L_EN, HIGH); digitalWrite(R_EN, HIGH);       // enabled + both PWM 0 = brake
  WiFi.mode(WIFI_STA); WiFi.disconnect();
  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  if (esp_now_init() != ESP_OK) { Serial.println("ERROR: esp_now_init failed, restarting"); delay(2000); ESP.restart(); }
  esp_now_register_recv_cb(onRecv);
  Serial.printf("Listening on channel %u, chair MAC %s\n", ESPNOW_CHANNEL, WiFi.macAddress().c_str());
  Serial.println("NOT ARMED: waiting for STOP from the box (or type a). Bench test: t");
  lastMs = millis();
}

void loop() {
  const uint32_t now = millis();
  const float dt = (now - lastMs) / 1000.0f; lastMs = now;

  // serial
  while (Serial.available()) {
    const char c = (char)Serial.read();
    if (c == 'f' || c == 'r' || c == 's') { serCmd = (char)(c - 32); serMs = now; testStep = -1; testL = testR = 0; }
    else if (c == 'a') { armed = true; printStatus("armed from Serial"); }
    else if (c == 't') { testStep = -1; testMs = now; testL = testR = 0; armed = false; speed = 0; Serial.println("[TEST] wheels OFF the ground!"); testStep = 99; }
    else if (c == '+') { capFwd = min(capFwd + 5.0f, 100.0f); printStatus("cap +"); }
    else if (c == '-') { capFwd = max(capFwd - 5.0f, 10.0f); printStatus("cap -"); }
    else if (c == '?') printStatus("status");
  }

  // guided bench test drives the sides directly and ignores the radio
  if (testStep != -1) {
    if (testStep == 99) testStep = -2;
    benchTest(now);
    drive(LEFT, testL); drive(RIGHT, testR);
    delay(5);
    return;
  }

  // radio
  portENTER_CRITICAL(&rxLock);
  const char cmd = rxCmd; const uint32_t ms = rxMs; const bool ever = rxEver;
  portEXIT_CRITICAL(&rxLock);
  const bool fresh = ever && (int32_t)(now - ms) >= 0 && (now - ms) <= LINK_TIMEOUT_MS;
  if (fresh != linkUp) { linkUp = fresh; if (!fresh) { armed = false; printStatus("LINK LOST -> stop, disarmed"); } else printStatus("link up"); }

  char want = 'S';
  if (serCmd && now - serMs <= SERIAL_CMD_MS) want = serCmd;        // serial bench drive (also needs armed)
  else { serCmd = 0; if (fresh) want = cmd; }
  if (want == 'S' && (fresh || serCmd) && !armed) { armed = true; printStatus("ARMED (got STOP)"); }
  if (!armed) want = 'S';

  ramp(targetFor(want), dt, now);
  drive(LEFT, speed); drive(RIGHT, speed);

  if (now - statMs >= 2000) { statMs = now; printStatus("status"); }
  delay(5);
}
