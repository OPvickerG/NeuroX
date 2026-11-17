/*
 * =============================================================================
 *  RC_Car_LR_Receiver.ino  -  r2-LR  -  ESP32 Dev Module on the car
 *  Listens for ESP-NOW broadcasts from NeuroX_LR_Sender and drives an L298N.
 *  Target : ESP32 (Dev Module), Arduino-ESP32 core 2.0.x or 3.x
 *  Serial : 115200 baud
 * -----------------------------------------------------------------------------
 *  Packet  : { 'N', 'X', cmd, seq }   (must match the sender exactly)
 *  Commands: 'F' -> both motors forward   (IN1=H IN2=L | IN3=H IN4=L)
 *            'R' -> both motors reverse   (IN1=L IN2=H | IN3=L IN4=H)
 *            '<' -> spin LEFT on the spot  (left wheel back, right wheel forward)
 *            '>' -> spin RIGHT on the spot (left wheel forward, right wheel back)
 *            'S' -> all IN pins LOW = brake (ENA/ENB are tied HIGH)
 *
 *  SAFETY
 *    Failsafe : no valid packet for 500 ms -> STOP. The sender repeats its
 *               command at least every 80 ms (every 25 ms right after a change),
 *               so this only trips if the sender resets, loses power or goes
 *               out of range (6+ packets lost in a row).
 *    Reversal : any change between two different movements (forward, reverse,
 *               left, right) first brakes for 150 ms. Reversing a spinning TT
 *               motor at full voltage draws roughly twice its stall current -
 *               the classic cause of brownouts.
 *    Brownout : the reset reason is printed at boot, so a battery that cannot
 *               hold the voltage shows up as "BROWNOUT" in the Serial Monitor.
 *
 *  L298N wiring (ENA / ENB tied HIGH = full speed):
 *    Left  motor: IN1 = GPIO 19, IN2 = GPIO 18
 *    Right motor: IN3 = GPIO 5,  IN4 = GPIO 21
 *    L298N GND must be connected to ESP32 GND.
 *
 *  Arduino IDE: keep this file in a folder named RC_Car_LR_Receiver.
 * =============================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_system.h>
#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>   // defines ESP_ARDUINO_VERSION_MAJOR (core 2.0.0+)
#endif

// ---- L298N pins ----
static const uint8_t IN1 = 19;   // Left motor
static const uint8_t IN2 = 18;   // Left motor
static const uint8_t IN3 = 5;    // Right motor
static const uint8_t IN4 = 21;   // Right motor

// ---- Radio (must match the sender) ----
static const uint8_t  ESPNOW_CHANNEL = 1;
static const uint8_t  PKT_MAGIC0     = 'N';
static const uint8_t  PKT_MAGIC1     = 'X';

// ---- Timing ----
static const uint32_t FAILSAFE_MS         = 500;   // no packet for this long -> STOP
static const uint32_t REVERSE_DEADTIME_MS = 150;   // brake time before changing direction
static const uint32_t STATUS_MS           = 2000;  // [STAT] line period
static const uint8_t  SEQ_GAP_RESTART     = 50;    // larger seq jump = sender rebooted, not loss

struct __attribute__((packed)) CarPacket {
  uint8_t magic0;
  uint8_t magic1;
  uint8_t cmd;      // 'F', 'R', '<', '>' or 'S'
  uint8_t seq;      // rolling counter: gaps = lost packets
};

// ---- Shared with the receive callback (it runs in the Wi-Fi task, on the other core) ----
// Every access goes through rxLock, so loop() always sees one consistent packet.
static portMUX_TYPE rxLock = portMUX_INITIALIZER_UNLOCKED;
static char     rxCmd      = 'S';
static uint32_t rxLastMs   = 0;
static bool     rxEver     = false;
static uint32_t rxGood     = 0;
static uint32_t rxBad      = 0;
static uint32_t rxLost     = 0;
static uint8_t  rxLastSeq  = 0;
static bool     rxSeqValid = false;

// ---- Loop state ----
static char     appliedCmd    = 0;      // 0 = nothing applied yet
static char     lastDir       = 0;      // last movement the motors actually ran ('F','R','<','>')
static uint32_t lastDirEndMs  = 0;      // when that direction was stopped
static bool     linkUp        = false;
static uint32_t lastStatusMs  = 0;

// =============================================================================
// MOTORS
// =============================================================================
static void allLow() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

static bool isDir(char c) { return c == 'F' || c == 'R' || c == '<' || c == '>'; }

static void applyCommand(char cmd, uint32_t now, const char *why) {
  if (isDir(appliedCmd) && cmd != appliedCmd) {   // a direction is ending: remember it
    lastDir = appliedCmd;
    lastDirEndMs = now;
  }
  allLow();                                        // never drive both directions at once
  switch (cmd) {
    case 'F':
      digitalWrite(IN1, HIGH);                     // left forward
      digitalWrite(IN3, HIGH);                     // right forward
      Serial.println("CMD: FORWARD  (IN1=H IN2=L | IN3=H IN4=L)");
      break;
    case 'R':
      digitalWrite(IN2, HIGH);                     // left reverse
      digitalWrite(IN4, HIGH);                     // right reverse
      Serial.println("CMD: REVERSE  (IN1=L IN2=H | IN3=L IN4=H)");
      break;
    case '<':
      digitalWrite(IN2, HIGH);                     // left reverse
      digitalWrite(IN3, HIGH);                     // right forward
      Serial.println("CMD: LEFT     (IN1=L IN2=H | IN3=H IN4=L) spin");
      break;
    case '>':
      digitalWrite(IN1, HIGH);                     // left forward
      digitalWrite(IN4, HIGH);                     // right reverse
      Serial.println("CMD: RIGHT    (IN1=H IN2=L | IN3=L IN4=H) spin");
      break;
    default:
      cmd = 'S';
      Serial.printf("CMD: STOP     (all IN pins LOW)%s\n", why);
      break;
  }
  appliedCmd = cmd;
}

// Moves the motors toward the wanted command, inserting the reversal brake.
static void driveToward(char wanted, uint32_t now) {
  if (wanted == appliedCmd) return;
  if (wanted == 'S') {
    applyCommand('S', now, "");
    return;
  }
  if (isDir(appliedCmd)) {                         // a different movement is running: brake first
    applyCommand('S', now, " - braking before changing direction");
    return;
  }
  if (isDir(lastDir) && lastDir != wanted && (uint32_t)(now - lastDirEndMs) < REVERSE_DEADTIME_MS) {
    return;                                        // still inside the brake time
  }
  applyCommand(wanted, now, "");
}

// =============================================================================
// ESP-NOW RECEIVE CALLBACK
// Kept short: validate, store under the lock, return. GPIO is driven from loop().
// =============================================================================
static void handlePacket(const uint8_t *data, int len) {
  const uint32_t now = millis();
  bool valid = (len == (int)sizeof(CarPacket));
  const CarPacket *p = (const CarPacket *)data;
  if (valid) valid = (p->magic0 == PKT_MAGIC0) && (p->magic1 == PKT_MAGIC1);
  if (valid) valid = (p->cmd == 'F') || (p->cmd == 'R') || (p->cmd == 'S') || (p->cmd == '<') || (p->cmd == '>');

  portENTER_CRITICAL(&rxLock);
  if (!valid) {
    rxBad = rxBad + 1;
  } else {
    if (rxSeqValid) {
      const uint8_t gap = (uint8_t)(p->seq - rxLastSeq - 1);
      if (gap > 0 && gap < SEQ_GAP_RESTART) rxLost = rxLost + gap;
    }
    rxLastSeq = p->seq;
    rxSeqValid = true;
    rxCmd = (char)p->cmd;
    rxLastMs = now;
    rxEver = true;
    rxGood = rxGood + 1;
  }
  portEXIT_CRITICAL(&rxLock);
}

#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
static void onRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  (void)info;
  handlePacket(data, len);
}
#else
static void onRecv(const uint8_t *mac, const uint8_t *data, int len) {
  (void)mac;
  handlePacket(data, len);
}
#endif

// =============================================================================
// SETUP / LOOP
// =============================================================================
static const char *resetReasonName(esp_reset_reason_t r) {
  switch (r) {
    case ESP_RST_POWERON:   return "power-on";
    case ESP_RST_EXT:       return "reset pin";
    case ESP_RST_SW:        return "software restart";
    case ESP_RST_PANIC:     return "CRASH (panic)";
    case ESP_RST_INT_WDT:   return "CRASH (interrupt watchdog)";
    case ESP_RST_TASK_WDT:  return "CRASH (task watchdog)";
    case ESP_RST_WDT:       return "CRASH (watchdog)";
    case ESP_RST_DEEPSLEEP: return "deep-sleep wake";
    case ESP_RST_BROWNOUT:  return "BROWNOUT (supply voltage dropped)";
    default:                return "other";
  }
}

void setup() {
  // Motor pins LOW first: the output latch is set before the pin becomes an output.
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  allLow();

  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println("=========================================");
  Serial.println(" RC Car ESP-NOW Receiver r2-LR (L298N, steering)");
  Serial.println("=========================================");

  const esp_reset_reason_t rr = esp_reset_reason();
  Serial.printf("Reset reason: %s\n", resetReasonName(rr));
  if (rr == ESP_RST_BROWNOUT) {
    Serial.println("WARNING: last reset was a BROWNOUT - the car battery could not hold the voltage");
    Serial.println("         when the motors started. Use a stronger battery (e.g. 2x 18650).");
  }

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_ps(WIFI_PS_NONE);                          // lowest latency
  if (esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE) != ESP_OK) {
    Serial.printf("WARNING: could not set Wi-Fi channel %u\n", (unsigned)ESPNOW_CHANNEL);
  }

  if (esp_now_init() != ESP_OK) {
    Serial.println("ERROR: esp_now_init failed - restarting in 3 s");
    delay(3000);
    ESP.restart();
  }
  esp_now_register_recv_cb(onRecv);

  Serial.printf("Listening on channel %u | receiver MAC %s\n", (unsigned)ESPNOW_CHANNEL,
                WiFi.macAddress().c_str());
  Serial.println("Waiting for sender... (motors stopped)");
  applyCommand('S', millis(), "");
}

void loop() {
  // 1) Snapshot the callback's data under the lock ...
  portENTER_CRITICAL(&rxLock);
  const bool     ever   = rxEver;
  const uint32_t lastMs = rxLastMs;
  const char     cmd    = rxCmd;
  const uint32_t good   = rxGood;
  const uint32_t bad    = rxBad;
  const uint32_t lost   = rxLost;
  portEXIT_CRITICAL(&rxLock);

  // 2) ... THEN read the clock, so lastMs can never be newer than now.
  const uint32_t now = millis();
  const bool fresh = ever && (uint32_t)(now - lastMs) <= FAILSAFE_MS;

  if (fresh && !linkUp) {
    linkUp = true;
    Serial.println("LINK: sender connected");
  } else if (!fresh && linkUp) {
    linkUp = false;
    Serial.println("FAILSAFE: no signal for 500 ms -> STOP");
  }

  driveToward(fresh ? cmd : 'S', now);

  if ((uint32_t)(now - lastStatusMs) >= STATUS_MS) {
    lastStatusMs = now;
    const uint32_t total = good + lost;
    Serial.printf("[STAT] link=%s motor=%c good=%lu lost=%lu (%.1f%%) bad=%lu\n", linkUp ? "UP" : "DOWN",
                  appliedCmd, (unsigned long)good, (unsigned long)lost,
                  total ? 100.0 * (double)lost / (double)total : 0.0, (unsigned long)bad);
  }

  delay(2);
}
