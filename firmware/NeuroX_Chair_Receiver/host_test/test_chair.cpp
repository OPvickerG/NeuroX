#include "Arduino.h"
#include "WiFi.h"
#include "esp_now.h"
uint32_t g_ms = 0; int g_dig[64]; uint32_t g_duty[64]; bool g_attached[64]; HWSerial Serial; WiFiClass WiFi; EspClass ESP; esp_now_recv_cb_t g_recv = nullptr;
#include "../NeuroX_Chair_Receiver.ino"
static int fails = 0, checks = 0; static uint8_t seq = 0;
#define CHECK(c, ...) do { ++checks; if (!(c)) { ++fails; printf("  FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)
static const uint32_t MAXD = (1u << PWM_BITS) - 1;
static void pkt(char c) { uint8_t d[4] = {'N', 'X', (uint8_t)c, seq++}; g_recv(nullptr, d, 4); }
// wheel speed as the chair sees it (+ = forward), from the PWM pins, undoing INVERT
static float wheel(const Side &s) { float v = ((float)g_duty[s.rpwm] - (float)g_duty[s.lpwm]) * 100.0f / MAXD; return s.invert ? -v : v; }
static float maxStep = 0, prevL = 0;
static bool shootThrough = false;
static void run(uint32_t ms, char send = 0, uint32_t every = 80) {
  for (uint32_t t = 0; t < ms; t += 5) {
    if (send && t % every == 0) pkt(send);
    loop();
    if (g_duty[L_RPWM] && g_duty[L_LPWM]) shootThrough = true;
    if (g_duty[R_RPWM] && g_duty[R_LPWM]) shootThrough = true;
    float l = wheel(LEFT); maxStep = std::max(maxStep, std::fabs(l - prevL)); prevL = l;
  }
}
int main() {
  Serial.quiet = true;
  setup();
  printf("1 boot: nothing moves, EN high (brake)\n");
  CHECK(wheel(LEFT) == 0 && wheel(RIGHT) == 0, "moving at boot"); CHECK(g_dig[L_EN] == 1 && g_dig[R_EN] == 1, "EN not high");
  printf("2 box already sending F at power-on: must NOT move (not armed)\n");
  run(1500, 'F'); CHECK(wheel(LEFT) == 0, "moved without arming: %.1f", wheel(LEFT));
  printf("3 STOP arms, then F ramps up to the cap, smoothly\n");
  run(300, 'S'); CHECK(armed, "not armed after S");
  run(300, 'F'); float early = wheel(LEFT);
  CHECK(early > 5 && early < MAX_DUTY_PCT, "ramp not gradual: %.1f after 0.3 s", early);
  run(1500, 'F'); CHECK(std::fabs(wheel(LEFT) - MAX_DUTY_PCT) < 0.6f && std::fabs(wheel(RIGHT) - MAX_DUTY_PCT) < 0.6f, "not at cap: L %.1f R %.1f", wheel(LEFT), wheel(RIGHT));
  CHECK(maxStep < 1.2f, "speed jumped %.2f %% in one 5 ms step", maxStep);
  printf("   right driver uses LPWM (mirrored motor): R %u L %u\n", (unsigned)g_duty[R_RPWM], (unsigned)g_duty[R_LPWM]);
  CHECK(g_duty[R_LPWM] > 0 && g_duty[R_RPWM] == 0, "right side not inverted");
  printf("4 STOP: down to 0 within 0.5 s\n");
  run(500, 'S'); CHECK(wheel(LEFT) == 0 && wheel(RIGHT) == 0, "still moving after STOP: %.1f", wheel(LEFT));
  printf("5 link lost while driving: stop within 0.6 s and disarm\n");
  run(1500, 'F'); CHECK(wheel(LEFT) > 50, "did not drive");
  run(600); CHECK(wheel(LEFT) == 0, "still moving after link loss: %.1f", wheel(LEFT)); CHECK(!armed, "still armed after link loss");
  printf("6 link back with F: stays stopped until STOP re-arms\n");
  run(800, 'F'); CHECK(wheel(LEFT) == 0, "moved after link came back without STOP");
  run(200, 'S'); run(1500, 'F'); CHECK(wheel(LEFT) > 50, "did not re-arm");
  printf("7 F -> R: passes zero, waits, reverse capped at %d %%\n", REV_DUTY_PCT);
  float minSeen = 999; uint32_t zeroAt = 0, revAt = 0;
  for (uint32_t t = 0; t < 2500; t += 5) { if (t % 80 == 0) pkt('R'); loop();
    float l = wheel(LEFT); if (l == 0 && !zeroAt) zeroAt = t; if (l < 0 && !revAt) revAt = t; minSeen = std::min(minSeen, l); }
  CHECK(zeroAt && revAt && revAt - zeroAt >= ZERO_DWELL_MS - 5, "no dwell at zero (zero %u, reverse %u)", zeroAt, revAt);
  CHECK(std::fabs(minSeen + REV_DUTY_PCT) < 0.6f, "reverse cap wrong: %.1f", minSeen);
  printf("8 bad packets are ignored\n");
  run(400, 'S'); uint32_t badBefore = rxBad; uint8_t junk[4] = {'N', 'X', 'Z', 0}; g_recv(nullptr, junk, 4); uint8_t shortp[3] = {'N', 'X', 'F'}; g_recv(nullptr, shortp, 3);
  uint8_t wrong[4] = {'N', 'H', 'F', 0}; g_recv(nullptr, wrong, 4); run(200, 'S');
  CHECK(rxBad == badBefore + 3 && wheel(LEFT) == 0, "bad packet accepted");
  printf("9 never both PWM pins on one driver\n");
  CHECK(!shootThrough, "RPWM and LPWM both on");
  printf("10 serial bench test 't' runs 6 steps and ends stopped\n");
  Serial.in.push_back('t'); bool sawL = false, sawR = false, bothAt = false;
  for (uint32_t t = 0; t < 16000; t += 5) { loop(); if (wheel(LEFT) != 0 && wheel(RIGHT) == 0) sawL = true; if (wheel(RIGHT) != 0 && wheel(LEFT) == 0) sawR = true; if (wheel(LEFT) > 0 && wheel(RIGHT) > 0) bothAt = true; }
  CHECK(sawL && sawR && bothAt, "test steps missing L%d R%d both%d", sawL, sawR, bothAt);
  CHECK(wheel(LEFT) == 0 && wheel(RIGHT) == 0 && testStep == -1, "test did not end stopped");
  printf("11 serial f runs max 2 s by itself\n");
  Serial.in.push_back('a'); Serial.in.push_back('f'); run(1500); CHECK(wheel(LEFT) > 20, "serial f did not drive");
  run(1500); CHECK(wheel(LEFT) == 0, "serial f did not time out");
  printf("\n%d checks, %d failed\n", checks, fails);
  return fails != 0;
}
