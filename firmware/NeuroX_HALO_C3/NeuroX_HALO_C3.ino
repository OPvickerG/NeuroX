/*
 * ============================================================================
 *  NeuroX HALO C3  v3.1  -  ESP32-C3 SuperMini in the HALO
 * ============================================================================
 *  Reads both BioAmp EXG Pills at exactly 256 Hz and streams the raw samples
 *  to the box (ESP32-S3) by ESP-NOW broadcast, 16 samples per packet
 *  (16 packets/s). The box does all the detection. No decisions here.
 *
 *  PINS (match the HALO C3 BOARD WIRING sheet)
 *    pill 2 OUT (forehead / nose, vertical)  -> GPIO 1
 *    pill 1 OUT (temples, horizontal)        -> GPIO 4
 *    battery divider 100k / 100k             -> GPIO 3
 *    on-board LED GPIO 8 (on = LOW): blinks while streaming
 *    Both pills on 3.3 V ONLY.
 *
 *  BOARD: Tools > Board > "ESP32C3 Dev Module", USB CDC On Boot: Enabled
 *  WORN = BATTERY ONLY. Nothing in either USB-C while it is on someone.
 * ============================================================================
 */
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

static const uint8_t PIN_VERT = 1, PIN_HORIZ = 4, PIN_VBAT = 3, PIN_LED = 8;
static const uint8_t ESPNOW_CHANNEL = 1;
static const uint8_t BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static const int8_t  RADIO_TX_POWER_QDBM = 60;     // 15 dBm
static const uint8_t ADC_OVERSAMPLE = 4;           // conversions averaged per sample (as in v13)

static const uint8_t HALO_SAMPLES = 16;
struct __attribute__((packed)) HaloPacket {        // keep in sync with NeuroX_Box_UI.ino
  uint8_t  magic0;  // 'N'
  uint8_t  magic1;  // 'H'
  uint8_t  ver;     // 1
  uint8_t  flags;   // bit0 = a sample hit an ADC rail
  uint16_t seq;
  uint16_t battMv;
  uint16_t vert[HALO_SAMPLES];
  uint16_t horiz[HALO_SAMPLES];
};

static HaloPacket pkt;
static uint8_t n = 0;
static uint16_t seq = 0, battMv = 0;
static uint32_t nextUs = 0, tick = 0, sent = 0, fail = 0, lastPrint = 0, lastBatt = 0;
static bool radioReady = false;

static uint16_t readAdc(uint8_t pin) {
  uint32_t acc = 0;
  for (uint8_t i = 0; i < ADC_OVERSAMPLE; ++i) acc += (uint32_t)analogRead(pin);
  return (uint16_t)(acc / ADC_OVERSAMPLE);
}

static void radioInit() {
  WiFi.mode(WIFI_STA); WiFi.disconnect();
  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_max_tx_power(RADIO_TX_POWER_QDBM);
  if (esp_now_init() != ESP_OK) { Serial.println("[RADIO] ERROR: esp_now_init failed"); return; }
  esp_now_peer_info_t peer; memset(&peer, 0, sizeof(peer));
  memcpy(peer.peer_addr, BROADCAST_MAC, 6); peer.channel = ESPNOW_CHANNEL; peer.ifidx = WIFI_IF_STA; peer.encrypt = false;
  if (esp_now_add_peer(&peer) != ESP_OK) { Serial.println("[RADIO] ERROR: could not add broadcast peer"); return; }
  radioReady = true;
  Serial.printf("[RADIO] streaming on ch %u, HALO MAC %s\n", (unsigned)ESPNOW_CHANNEL, WiFi.macAddress().c_str());
}

void setup() {
  Serial.begin(115200); delay(300);
  Serial.println("\nNeuroX HALO C3 v3.1");
  pinMode(PIN_LED, OUTPUT); digitalWrite(PIN_LED, HIGH);
  analogReadResolution(12);
  // core 3.x only accepts per-pin attenuation once the pin is attached to the ADC: read once first
  (void)analogRead(PIN_VERT); (void)analogRead(PIN_HORIZ); (void)analogRead(PIN_VBAT);
  analogSetPinAttenuation(PIN_VERT, ADC_11db); analogSetPinAttenuation(PIN_HORIZ, ADC_11db); analogSetPinAttenuation(PIN_VBAT, ADC_11db);
  battMv = (uint16_t)(analogReadMilliVolts(PIN_VBAT) * 2);
  radioInit();
  pkt.magic0 = 'N'; pkt.magic1 = 'H'; pkt.ver = 1; pkt.flags = 0;
  nextUs = micros();
}

void loop() {
  // exactly 256.000 Hz: 3906 us, plus 1 us every 4th tick (1e6 / 256 = 3906.25)
  if ((int32_t)(micros() - nextUs) < 0) return;
  nextUs += 3906 + ((tick & 3) == 3 ? 1 : 0); ++tick;

  const uint16_t v = readAdc(PIN_VERT), h = readAdc(PIN_HORIZ);
  pkt.vert[n] = v; pkt.horiz[n] = h;
  if (v <= 16 || v >= 4079 || h <= 16 || h >= 4079) pkt.flags |= 1;
  if (++n >= HALO_SAMPLES) {
    pkt.seq = seq++; pkt.battMv = battMv;
    if (radioReady && esp_now_send(BROADCAST_MAC, (const uint8_t *)&pkt, sizeof(pkt)) == ESP_OK) ++sent; else ++fail;
    n = 0; pkt.flags = 0;
    digitalWrite(PIN_LED, (seq & 8) ? LOW : HIGH);           // ~1 Hz blink = streaming
  }

  const uint32_t ms = millis();
  if (ms - lastBatt >= 1000) {                               // between samples, ~0.1 ms
    lastBatt = ms;
    const uint16_t mv = (uint16_t)(analogReadMilliVolts(PIN_VBAT) * 2);
    battMv = (uint16_t)((battMv * 3 + mv) / 4);
  }
  if (ms - lastPrint >= 2000) {
    lastPrint = ms;
    Serial.printf("[HALO] vert %4u  horiz %4u  batt %u mV  packets %lu ok / %lu fail%s\n", v, h, battMv,
                  (unsigned long)sent, (unsigned long)fail, (v <= 16 || v >= 4079 || h <= 16 || h >= 4079) ? "  CLIPPED" : "");
  }
  // fell far behind (Serial, Wi-Fi)? resync instead of bursting
  if ((int32_t)(micros() - nextUs) > 50000) nextUs = micros();
}
