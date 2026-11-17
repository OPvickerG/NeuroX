/*
 * =============================================================================
 *  NeuroX_Final_Sender.ino  -  firmware v13.2-ESPNOW-r3-LR2-FINAL (auto-calibration + 3 s lock + PRO)
 *
 *  FINAL (on top of PRO-SAFE; tuned on 100 virtual people of every kind -
 *  typical, weak blink, facial palsy, weak jaw, tremor, spasms, frequent
 *  blinking, slow / elderly, noisy electrodes, nystagmus, very strong - and
 *  checked on 100 more it had never seen):
 *    - AUTO-CALIBRATION at every power-on - no keyboard needed. Follow the beeps:
 *        sit still -> BLINK at each beep (5x) -> CLENCH at each beep (5x)
 *        -> close your eyes FIRMLY at the beep, relax at the double beep (2x).
 *      Only answers right after a beep are recorded. Unclear -> it asks once
 *      more, then keeps your last good calibration (or factory settings).
 *      Again any time: NEW button (GPIO 7), hold BOOT 1 s, or key t / a.
 *    - NOISE TRACKING: the resting noise is measured while you sit still and
 *      followed afterwards (only on a calm signal); the blink trigger, the
 *      talking filter, the look-down check, the slow-bite trigger and the
 *      unlock level always stay above it (loose / dry electrodes, tremor, hum).
 *    - UNLOCK: close your eyes firmly 3 s. Anti-yawn: the 3 s must also
 *      average >= 35 % of your calibration squeeze, and the forehead/temple
 *      ratio limit is 2x your clench's. Squeeze too weak to see (facial palsy)
 *      -> calibration switches unlock to BITE AND HOLD 3 s (same anti-yawn
 *      check on the temples). The unlock level never rises above 60 % of
 *      your squeeze, so it always stays reachable.
 *    - Blinks: slow blinks (no dip within 0.2 s), blinks that start inside the
 *      previous blink's dip, a clench right after a blink, a slow 2nd blink of
 *      a double blink - all recognised. An everyday blink just before a
 *      command blink is not a double blink (the two blinks of a pair must be
 *      similar in size).
 *    - Your bite length sets the double-clench window and the talking filter.
 *    - Relock after 3 s never cuts a gesture that is being measured.
 *    - STOP button (GPIO 15, to GND): stops and LOCKS at once.
 *    Same electrodes, same car receiver (RC_Car_LR_Receiver).
 *
 *  SAFETY LOCK (SAFE):
 *    - Starts LOCKED: the car only ever receives STOP, gestures are ignored.
 *    - UNLOCK: squeeze your eyes shut HARD and hold 3 s ("[LOCK] holding... 1 s, 2 s").
 *    - ACTIVE: blinks / clenches drive the car exactly as in the LR2-PRO version
 *      (same 4 gestures, same talking / look-down / soft-blink filters).
 *    - 3 s without a command -> LOCKED again (the car is already stopped:
 *      every command is a short 0.42 s pulse).
 *    - Eating, talking and coughing are short bursts: they cannot hold 3 s.
 *    - Training 't' has a 3rd step that measures YOUR squeeze for the unlock.
 *    - Keyboard: o = lock / unlock, f b l r = test pulses (work in every state).
 *    The car receiver is unchanged (LOCKED simply means: only STOP is sent).
 *    Buzzer (optional): + to GPIO 13, - to GND.
 *
 *  PRO (on top of the competition LR2 version, gesture engine unchanged):
 *    - talking / chewing filter: clench commands only from a quiet jaw (1 or 2 bites)
 *    - a double clench the watcher missed becomes TURN RIGHT instead of FORWARD
 *    - TURN RIGHT is confirmed 0.9 s after the 2nd bite (no 3rd bite = not talking)
 *    - looking down/up at the car is not read as a blink (REVERSE)
 *    - a clench right after a right turn is no longer swallowed (2.5 s -> 1.2 s)
 *    - after personal training ('t'): an everyday blink under 65 % of your trained
 *      blink is not a command (set FIRM_BLINK_ENABLED = false to switch off)
 *    Bite ~0.5-1 s and relax; a bite held 1.7 s+ counts as talking.
 *    Same electrodes as the competition, same receiver (RC_Car_LR_Receiver r2).
 *
 *  STEERING (LR): double gestures, decided after the gesture engine:
 *    single blink   -> REVERSE   'R'  (sent ~1 s later: waits for a 2nd blink)
 *    double blink   -> TURN LEFT '<'  ("blink-blink", 2nd blink 0.25 - 1 s later, may be softer)
 *    single clench  -> FORWARD   'F'  (sent ~1.5 s later: waits for a 2nd clench)
 *    double clench  -> TURN RIGHT '>' (bite, relax, bite again within ~1.5 s)
 *    The 2nd gesture is seen by a small watcher that runs on every sample,
 *    because a natural double blink lands inside the engine's refractory time.
 *    A turn spins the car on the spot (one wheel forward, one back) for
 *    turnPulseMs (default 300 ms, keys [ / ] change it by 50 ms).
 *  = EOG_EMG_RC_Car.ino v13.2 with ONLY the motor output changed:
 *    the GPIO5/6/7/8 motor + steering lines are gone, and the FORWARD / REVERSE
 *    pulses are broadcast over ESP-NOW instead. Every DSP filter, threshold,
 *    gesture rule, timing value, calibration step and hotkey is identical to v13.2
 *    until you run personal training ('t', see below); 'u' returns to v13.2.
 *
 *  RADIO: ESP-NOW broadcast to FF:FF:FF:FF:FF:FF, Wi-Fi channel 1.
 *    Packet = { 'N', 'X', cmd, seq }   cmd = 'F' forward | 'R' reverse | 'S' stop
 *    v13.2 "GPIO5 HIGH for 420 ms"  ->  'F' for 420 ms, then 'S'
 *    v13.2 "GPIO6 HIGH for 420 ms"  ->  'R' for 420 ms, then 'S'
 *    all GPIO LOW (idle/cooldown)   ->  'S'
 *    Every NEW command is sent 3 times on consecutive 256 Hz ticks (~4 ms
 *    apart), then repeated every 25 ms for 250 ms, because broadcast packets
 *    get no ACK and no automatic retry.
 *    The current command is also re-sent every 80 ms (heartbeat) so the car's
 *    500 ms failsafe stops it if this board resets or goes out of range
 *    (6 heartbeats must be lost in a row before the failsafe trips).
 *    Packets are only transmitted right after a sample has been read, so the
 *    Wi-Fi current burst on the shared 3.3 V rail (which also powers both
 *    BioAmp pills) never overlaps an ADC conversion.
 *
 *  PERSONAL TRAINING (r3): press 't' and follow the prompts (5 blinks, then
 *    5 jaw clenches). The firmware measures YOUR blink shape and YOUR muscle
 *    (EMG) levels and sets every base threshold from them:
 *      - jaw EMG level AND duration: a blink's eyelid-muscle burst can
 *        exceed the factory jaw level (70) for a few samples and turn blinks
 *        into FORWARD; a real clench stays above it for most of the window.
 *        Training picks the EMG source (both pills as in v13.2, temples
 *        only, or forehead only), level and sample count that best separate
 *        YOUR blinks from YOUR clenches,
 *      - blink thresholds at ~60 % of your real blink, so normal blinks pass.
 *    The result is saved in flash and loaded at every boot. 'u' returns to the
 *    factory v13.2 thresholds. Untrained, the firmware is exactly v13.2.
 *
 *  2-channel EOG/EMG "blink & bite" brain-computer interface -> RC car
 *  Target : ESP32-S3, Arduino-ESP32 core 2.0.x or 3.x
 *  Serial : 115200 baud (UART0 or native USB CDC - both handled non-blocking)
 * -----------------------------------------------------------------------------
 *  GESTURES  (V shape from GPIO1 forehead/jaw; EMG = max(GPIO1 EMG, GPIO4 temple EMG))
 *    JAW CLENCH -> FORWARD pulse ('F' over ESP-NOW), if EITHER
 *      a) EMG > 70 on >= 4 samples of the window (sustained muscle bite), or
 *      b) winVMax >= +235 and winVMin >= -55 and winVMax > 3.2 x |dip|
 *         (strong monophasic positive shift without a deep blink trough).
 *    EYELID BLINK -> REVERSE pulse ('R' over ESP-NOW), if EITHER
 *      a) winVMin <= -135 (negative trough), or
 *      b) winVMax >= +115 and P2P >= 200 and the return after the peak <= -50.
 *    BOTH matched -> FORWARD if EMG was above threshold on >= 6 samples,
 *      otherwise the side with the larger normalised evidence wins.
 *    A capture opened by a slow DC shift (no sharp onset) can only become
 *    FORWARD through the EMG rule and never fires REVERSE.
 *
 *  DSP CHAIN (both channels, every 256 Hz tick, horizontal then vertical)
 *    ADC (12-bit, 11 dB, 4 conversions averaged) - DC offset
 *      +-> EMG: pop rejection -> 50 Hz notch -> HP 15 Hz (4th order)
 *               -> |x| -> 32-sample moving average                         = emg
 *      -> 50 Hz notch (Direct Form II Transposed biquad, Q = 4.5)
 *      -> 5-sample running median (despike)
 *           +-> 4-stage cascade HP 0.5 Hz x2 + LP 12 Hz x2 (Butterworth)     = eog
 *           +-> LP 2 Hz DC-coupled level (bite-hold latch only)             = level
 *
 *  GESTURE STATE MACHINE   ST_ARMED -> ST_CAPTURE -> ST_SETTLE -> ST_ARMED
 *    ARMED   : 10-sample pre-trigger ring always running. A capture opens on a
 *              SHARP excursion |V| >= trigger (the edge must have moved >=
 *              onsetRise within the 39 ms pre-trigger ring, so slow filter
 *              tails / drift never open a capture), on EMG > 70 while
 *              V >= +40, or on a slow positive DC shift (slow clench, chewing)
 *              so the EMG rule can still judge it; if it is not a bite, the
 *              shift is latched so its release is absorbed.
 *    CAPTURE : window = 10 pre-trigger + 40 post-trigger samples (~195 ms).
 *              The decision is made only once the full window has been seen
 *              (no mid-window exits). Window features are measured from the
 *              pre-onset baseline, so a gesture starting on a filter tail is
 *              judged fairly.
 *    SETTLE  : >= 220 ms dead time AND 10 consecutive quiet samples
 *              (|V| < 85 and EMG < 65). If the event shifted the DC level
 *              (a FORWARD bite, or a bite too weak to be one), it also waits
 *              until the bite is released plus 640 ms: through the 0.5 Hz
 *              high-pass, a held bite's undershoot and its release both swing
 *              negative and would otherwise be read as a blink.
 *
 *  SENSITIVITY: thrScale (1/2/3, +/-, 0) multiplies every amplitude threshold;
 *  biteScale (j/k) additionally moves only the FORWARD thresholds (and the
 *  matching re-arm EMG gate). REVERSE thresholds never follow biteScale.
 *
 *  HOTKEYS: 1/2/3 presets | + - fine tune | 0 reset | j k jaw-clench sensitivity |
 *           f b test pulse | s stop+clear+re-arm | c recalibrate | p plotter<->text | h help
 *           t personal training | u factory v13.2 thresholds
 *           l / r test turn LEFT / RIGHT | [ / ] turn time -/+ 50 ms
 *
 *  Arduino IDE: keep this file in a folder named NeuroX_Final_Sender.
 * =============================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_system.h>
#include <Preferences.h>
#include <math.h>
#include <algorithm>
#if __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// =============================================================================
// 1. HARDWARE
// =============================================================================
static const uint8_t PIN_HORIZ = 4;   // Pill 1: temples L/R  (telemetry only)
static const uint8_t PIN_VERT  = 1;   // Pill 2: forehead/jaw (EOG + EMG, decisions)

static const uint8_t ADC_BITS       = 12;
static const uint8_t ADC_OVERSAMPLE = 4;        // conversions averaged per sample (1 = off)
static const float   ADC_RAIL_LOW   = 16.0f;    // counts: at/below = clipped low
static const float   ADC_RAIL_HIGH  = 4079.0f;  // counts: at/above = clipped high

// =============================================================================
// 1b. ESP-NOW RADIO (replaces the v13.2 motor GPIO pins)
//    The receiver must use the same channel and magic bytes.
// =============================================================================
static const uint8_t  ESPNOW_CHANNEL   = 1;
static const uint8_t  BROADCAST_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static const uint8_t  PKT_MAGIC0       = 'N';
static const uint8_t  PKT_MAGIC1       = 'X';
static const uint32_t HEARTBEAT_MS     = 80;    // re-send current command; >= 6 packets per 500 ms receiver failsafe
static const uint8_t  RADIO_REPEATS    = 2;     // each NEW command goes out 1 + 2 times, ~4 ms apart
static const uint32_t FAST_HEARTBEAT_MS   = 25;   // ...then every 25 ms for 250 ms after any change,
static const uint32_t FAST_HEARTBEAT_SPAN = 250;  // so a lost start / STOP costs <= 25 ms, not 80 ms
// Max Wi-Fi TX power, units of 0.25 dBm (ESP-IDF esp_wifi_set_max_tx_power).
// 60 = 15 dBm: far more than a car a few metres away needs, with less RF and
// less supply ripple next to the electrodes than the 20 dBm default.
// If the car ever reports lost packets at range, raise to 80 (20 dBm).
static const int8_t   RADIO_TX_POWER_QDBM = 60;

struct __attribute__((packed)) CarPacket {
  uint8_t magic0;
  uint8_t magic1;
  uint8_t cmd;      // 'F', 'R' or 'S'
  uint8_t seq;      // rolling counter (diagnostics only)
};

// =============================================================================
// 2. TIMING: exactly 256.000 Hz
//    1e6 / 256 = 3906.25 us. The tick period is 3906 us plus one extra
//    microsecond every 4th tick (3906, 3906, 3906, 3907, ...), accumulated
//    Bresenham-style, so the long-run rate is exactly 256.000 Hz with zero drift.
//    (A strict 3907/3906 alternation would average 3906.5 us = 255.97 Hz.)
// =============================================================================
static const uint32_t FS_HZ          = 256;
static const float    FS_F           = 256.0f;
static const uint32_t TICK_BASE_US   = 1000000UL / FS_HZ;   // 3906
static const uint32_t TICK_FRAC_NUM  = 1000000UL % FS_HZ;   // 64 -> +1 us every 256/64 = 4 ticks
static const uint32_t RESYNC_LATE_US = 2UL * TICK_BASE_US;  // > 2 ticks late = overrun -> resync

// =============================================================================
// 3. DSP
// =============================================================================
static const double NOTCH_HZ     = 50.0;
static const double NOTCH_Q      = 4.5;
static const double BP_LOW_HZ    = 0.5;
static const double BP_HIGH_HZ   = 12.0;
static const double EMG_HP_HZ    = 15.0;
static const double LEVEL_LP_HZ  = 2.0;
static const double BUTTER4_Q1   = 0.54119610014619698;   // 4th-order Butterworth, pole pair 1
static const double BUTTER4_Q2   = 1.30656296487637652;   // 4th-order Butterworth, pole pair 2
static const double BUTTER2_Q    = 0.70710678118654752;   // 2nd-order Butterworth
static const double DSP_PI       = 3.14159265358979323846;
static const uint8_t MEDIAN_LEN  = 5;
static const uint8_t EMG_MA_LEN  = 32;                    // must be a power of two
// The EMG branch is taken BEFORE the 5-point median: the median strips ~65 % of
// the EMG envelope at 256 Hz (it crushed live clench EMG to 7..28).
static const bool EMG_TAP_BEFORE_MEDIAN = true;
// Pop rejection for the un-medianed EMG branch. A single-sample electrode pop
// rings the 50 Hz notch for ~10 samples and the 32-sample moving average then
// stretches it into ~30 samples above threshold, so a ">= 4 samples" rule alone
// cannot reject it. The EMG branch therefore drops isolated outliers BEFORE its
// own notch (Hampel test against a 5-sample running median of the raw signal).
// Real EMG rarely deviates this far from its local median, so bites are unaffected.
static const float EMG_POP_REJECT = 500.0f;     // raw counts

// =============================================================================
// 4. CALIBRATION / WARM-UP
// =============================================================================
static const uint16_t BOOT_DELAY_SAMPLES = 256;   // 1 s for front-ends and user to settle
static const uint16_t CAL_SAMPLES        = 512;   // 2 s @ 256 Hz
static const uint16_t WARMUP_SAMPLES     = 512;   // 2 s filter warm-up after (re)calibration
static const float    CAL_NOISY_SD       = 50.0f; // raw-count SD above this = poor contact / movement
static const float    CAL_RAIL_MARGIN    = 100.0f;

// =============================================================================
// 5. GESTURE ENGINE
// =============================================================================
static const uint8_t  PRE_TRIG_LEN        = 10;   // ~39 ms pre-trigger history
static const uint8_t  CAPTURE_POST_LEN    = 40;   // post-trigger samples incl. trigger sample (~156 ms)
static const uint8_t  EMG_MIN_COUNT       = 4;    // FORWARD by EMG: combEmg > fwdEmg on >= 4 window samples
static const uint8_t  EMG_STRONG_COUNT    = 6;    // conflict: >= 6 samples = sustained bite, FORWARD wins
// FORWARD shape: winVMax > 3.2 x |negative dip|. With the current numbers this is
// implied by winVMax >= 235 and winVMin >= -55 (3.2 x 55 = 176 < 235); it only
// starts to bite if those two thresholds are retuned.
static const float    FWD_DOMINANCE       = 3.2f;
static const uint32_t REFRACTORY_SAMPLES  = (220UL * FS_HZ + 999UL) / 1000UL;  // 57 samples = 222.7 ms
static const uint8_t  QUIET_RUN_REQUIRED  = 10;   // consecutive quiet samples before re-arm
static const uint32_t SETTLE_WARN_SAMPLES = 5UL * FS_HZ;   // "stuck in SETTLE" warning period
static const uint32_t BITE_LATCH_MAX      = 15UL * FS_HZ;  // bite-hold latch safety timeout (DC drift)
static const float    BITE_RELEASE_FRAC   = 0.5f;          // released when DC shift < 50 % of its peak
static const float    LEVEL_BASE_ALPHA    = 1.0f / FS_F;   // DC-level baseline tracker, time constant 1 s
static const uint16_t BITE_RELEASE_HOLDOFF = 164; // 640 ms: absorbs the 0.5 Hz high-pass rebound of the release
static const uint16_t CLIP_HOLDOFF_SAMPLES = 256; // 1 s re-arm delay after any sample on an ADC rail
static const uint16_t SAT_TRIP_SAMPLES    = 32;   // 125 ms on the rail -> saturated (electrode off)
static const uint16_t SAT_CLEAR_SAMPLES   = 128;  // 500 ms off the rail -> cleared (filters reset + 2 s hold-off)

// Base thresholds (filtered ADC counts) at scale x1.00.
// Every amplitude is multiplied by thrScale (1/2/3, +/-, 0). The FORWARD
// thresholds and the re-arm EMG gate are additionally adjusted by biteScale
// (j/k), which never touches the REVERSE rules.
static const float BASE_TRIG_V        = 110.0f;   // |V| that opens a capture (> quiet 85 = hysteresis)
static const float BASE_ONSET_RISE    = 66.0f;    // edge must move this much inside the pre-trigger ring
static const float BASE_EMG_TRIG_VMIN = 40.0f;    // EMG-backed trigger also needs V >= +40
// FORWARD (jaw clench), either:
//   a) combEmg > FWD_EMG on >= EMG_MIN_COUNT window samples, or
//   b) winVMax >= FWD_VMAX and winVMin >= FWD_MIN_FLOOR and winVMax > 3.2 x |min(0, winVMin)|
static const float BASE_FWD_EMG       = 70.0f;    // x thrScale x biteScale
static const float BASE_FWD_VMAX      = 235.0f;   // x thrScale x biteScale
static const float BASE_FWD_MIN_FLOOR = -55.0f;   // x thrScale / biteScale (more sensitive = deeper dip allowed)
// REVERSE (eyelid blink), either:
//   a) winVMin <= REV_DEEP_MIN, or
//   b) winVMax >= BI_VMAX and P2P >= BI_P2P and retMin (after the peak) <= BI_RET_MIN
static const float BASE_REV_DEEP_MIN  = -135.0f;   // v13.1: was -175 (catch normal blinks)
static const float BASE_BI_VMAX       = 115.0f;    // v13.2: was 185 (softer blink, ~129 peak)
static const float BASE_BI_P2P        = 200.0f;    // v13.2: was 340
static const float BASE_BI_RET_MIN    = -50.0f;    // v13.1: was -80
static const float BASE_QUIET_V       = 85.0f;    // SETTLE quiet: |V| < 85
static const float BASE_QUIET_EMG     = 65.0f;    // SETTLE quiet: combEmg < 65, x thrScale x biteScale
static const float BASE_BITE_LEVEL    = 100.0f;   // DC shift vs 1 s baseline that marks a held bite
static const float BASE_BITE_LATCH_MIN = 40.0f;   // DC rise at window end that latches a bite
static const float BASE_DEEP_PEAK_MIN  = -1.0e9f;  // dip rule: required upward phase (v13.2: none)

static const float SCALE_SENSITIVE = 0.85f;
static const float SCALE_DEFAULT   = 1.00f;
static const float SCALE_STRICT    = 1.35f;
static const float SCALE_STEP      = 0.10f;
static const float SCALE_MIN       = 0.40f;
static const float SCALE_MAX       = 3.00f;
static const float BITE_SCALE_STEP = 0.10f;      // j = 10 % more sensitive, k = 10 % less
static const float BITE_SCALE_MIN  = 0.50f;
static const float BITE_SCALE_MAX  = 2.00f;

// Personal training ('t')
static const uint8_t  TRAIN_SAMPLES          = 5;       // blinks, then clenches
static const uint32_t TRAIN_TIMEOUT_MS       = 45000;   // per step
static const float    TRAIN_CAPTURE_SCALE    = 0.70f;   // capture opening x0.70 while training
static const float    TRAIN_BLINK_FRAC       = 0.60f;   // blink thresholds = 60 % of your median blink
static const float    TRAIN_RET_FRAC         = 0.50f;   // return-after-peak threshold = 50 % of yours
static const float    TRAIN_BLINK_EMG_MARGIN = 1.15f;   // fallback: jaw threshold 15 % above blink EMG
static const float    TRAIN_EMG_FLOOR        = 35.0f;   // never set the jaw threshold below this
static const uint32_t TRAIN_MAGIC            = 0x4E583330UL;  // "NX30": saved record (+ bite unlock + bite length)
// ---- FINAL: auto-calibration ----
static const bool     AUTO_CAL_AT_BOOT       = true;    // calibrate by itself at every power-on
static const uint32_t AC_BOOT_DELAY_MS       = 800;     // after [READY]
static const uint32_t AC_REST_MS             = 3000;    // sit still: noise measurement (last 2 s)
static const uint32_t AC_REST_SETTLE_MS      = 1000;    // first 1 s: let the previous step fade
static const uint16_t AC_REST_BUF            = 520;     // 2 s of samples for the exact noise level
static const uint32_t AC_REST_WAIT_MS        = 3000;    // ... measured once the signal is calm (max 3 s wait)
static const uint32_t AC_REST_MAX_MS         = 7000;
static const float    AC_CAPTURE_SCALE       = 0.50f;   // capture very sensitively (only answers to a beep count)
static const uint32_t AC_BLINK_WIN_MS        = 2600;    // answer window after a BLINK beep
static const uint32_t AC_CLENCH_WIN_MS       = 3400;    // answer window after a CLENCH beep
static const uint32_t AC_MIN_REACT_MS        = 120;     // faster than this = not an answer to the beep
static const uint32_t AC_GAP_MS              = 700;     // pause before the next beep
static const uint8_t  AC_MAX_PROMPTS         = 9;       // beeps per step
static const uint8_t  AC_MIN_SAMPLES         = 3;       // fewer answers than this = step failed
static const uint8_t  AC_GOOD_OK             = 4;       // self-check: 4 of 5 = good
static const uint8_t  AC_MIN_OK              = 3;       // 3 of 5 = usable
static const float    AC_CLEAR_BLINK_K       = 5.0f;    // a blink this many x the eye noise ends its window early
static const float    AC_CLEAR_BLINK_MIN     = 120.0f;
static const uint16_t AC_WIN_MAX             = 1000;
// ---- FINAL: noise floors ----
static const bool     NOISE_FLOORS_ENABLED   = true;
static const uint32_t NZ_FAST_SAMPLES        = 2560;    // first 10 s: follow the noise quickly
static const uint32_t NZ_MIN_SAMPLES         = 512;
static const float    NZ_STEP_FAST           = 0.003f;
static const float    NZ_STEP_SLOW           = 0.0002f;
static const float    NZ_QUIET_V_K           = 2.2f;    // quiet eye signal >= 2.2 x resting level
static const float    NZ_TRIG_K              = 2.8f;    // blink trigger    >= 2.8 x resting level
static const float    NZ_QUIET_EMG_K         = 1.8f;
static const float    NZ_FWD_EMG_K           = 2.2f;    // jaw rule         >= 2.2 x resting muscle noise
static const float    NZ_DC_K                = 2.5f;    // slow-bite trigger    >= 2.5 x electrode drift
static const float    SLOW_BLINK_TRIG_K      = 1.3f;    // slow blink: 1.3 x the trigger ...
static const uint8_t  SLOW_RING_LEN          = 32;      // ... and a rise of 1 x the trigger within 125 ms
static const uint16_t NZ_ARMED_SETTLE        = 256;     // noise counts only after 1 s armed
static const float    DIP_RISE_K             = 1.5f;    // blink out of a dip: rise >= 1.5 x trigger ...
static const float    DIP_RISE_END_K         = 0.6f;    // ... ending at >= 0.6 x trigger
static const float    FIRM_NOISE_K           = 3.0f;    // firm blink: median swing - 3 x eye noise at most
static const float    NZ_UNLOCK_K            = 2.2f;    // unlock level     >= 2.2 x resting forehead noise
static const uint32_t UNLOCK_RELEASE_MAX_MS  = 2500;    // unlocked: go ACTIVE after this even if the forehead stays noisy
static const float    UNLOCK_MAX_SQZ_FRAC    = 0.60f;   // unlock level never above 60 % of your squeeze
static const float    UNLOCK_SEP_REST        = 1.3f;    // ... and that must still be 1.3 x your resting level
static const float    BITE_UNLOCK_FRAC       = 0.50f;   // bite-and-hold unlock: 50 % of your clench (temples)
static const float    BLINK2_FIRM_FRAC       = 0.70f;
static const float    BLINK_PAIR_FRAC        = 0.72f;   // a double blink = two similar blinks (1st >= 72 % of the 2nd)
static const float    BLINK_PAIR_NOISE_K     = 3.5f;
static const float    BLINK2_RISE_K          = 1.25f;
static const uint32_t BLINK_EXTEND_MS        = 600;
static const uint16_t WATCH_JAW_MAX          = 10;
static const uint16_t SETTLE_JAW_SAMPLES     = 12;      // 47 ms of jaw EMG after a blink re-arms for the clench      // 2nd blink with > 39 ms of jaw EMG = a clench     // double-blink window waits for a blink in progress   // 2nd blink from the 1st one's dip: rise >= 1.25 x the firm peak    // ... and the difference must be bigger than the eye noise (tremor)
static const uint16_t WATCH_SIZE_SAMPLES     = 72;      // 2nd blink: its dip is followed (max 0.28 s) to measure its size   // calibrated: 2nd blink of a double blink >= 70 % of your blink
// ---- FINAL: buttons (to GND; unconnected = not pressed) ----
static const bool     BUTTONS_ENABLED        = true;
static const uint8_t  PIN_BTN_STOP           = 15;      // STOP: stop + LOCK
static const uint8_t  PIN_BTN_NEW            = 7;       // NEW person: calibrate again
static const uint8_t  PIN_BTN_BOOT           = 0;       // BOOT button on the DevKit: hold 1 s = calibrate again
static const float    TRAIN_DEEP_PEAK_FRAC   = 0.35f;   // dip rule needs an upward phase >= 35 % of your blink peak
static const uint8_t  TRAIN_MIN_GAP          = 6;       // "good" = clench exceeds blink by >= 6 window samples
static const float    TRAIN_BLINK_STRESS     = 1.25f;   // pick the jaw level as if blinks were 25 % stronger ...
static const float    TRAIN_CLENCH_STRESS    = 0.80f;   // ... and clenches 20 % weaker than in training
static const float    TRAIN_STRONG_FRAC      = 0.80f;   // EMG beats a blink shape only if it lasts 80 % of your weakest clench
static const float    RULE_OFF               = 1.0e9f;  // a threshold no signal can reach = rule switched off
// Buzzer: connect + to GPIO 13 and - to GND (passive or active buzzer, 3.3 V).
static const bool     BUZZER_ENABLED         = true;
static const uint8_t  PIN_BUZZER             = 13;
static const uint32_t BUZZ_FREQ_HZ           = 2048;    // = 8 x 256 Hz sample rate (ripple lands at 0 Hz)
static const uint32_t BUZZ_DUTY              = 128;     // 50 % of 8-bit
static const uint8_t  BUZZ_LEDC_CH           = 2;       // (core 2.x only)
static const uint32_t BUZZ_ARTIFACT_MS       = 100;     // ignore eye movements for 0.1 s after a beep
static const uint32_t HOLD_TICK_DELAY_MS     = 300;     // a hold must last 0.3 s before the first tick
static const float    LOCK_METER_FRAC        = 0.35f;   // 'waiting' line while forehead > 35 % of the unlock level
static const uint32_t LOCK_METER_MS          = 500;     // ... twice per second
static const uint16_t BZ_TICK[]   = {50};               // hold progress
static const uint16_t BZ_WEAK[]   = {25, 70, 25};       // seen, but not counting yet
static const uint16_t BZ_UNLOCK[] = {80, 60, 80, 60, 260};   // UNLOCKED / training done
static const uint16_t BZ_LOCK[]   = {450};              // LOCKED
static const uint16_t BZ_BROKEN[] = {160, 110, 160};    // hold broken / no eye closing
static const uint16_t BZ_READY[]  = {60, 90, 60};       // system ready
static const uint16_t BZ_CLOSE[]  = {120};              // training: close now (short: closing starts after it)
static const uint16_t BZ_RELAX[]  = {60, 80, 60};       // training: relax / open
static const uint16_t BZ_REC[]    = {30};               // training: gesture recorded
// Safety lock
#ifndef UNLOCK_FACTORY_LEVEL_SET
#define UNLOCK_FACTORY_LEVEL_SET 30.0f
#endif
#ifndef UNLOCK_FACTORY_RATIO_SET
#define UNLOCK_FACTORY_RATIO_SET 0.0f
#endif
static const uint32_t UNLOCK_HOLD_MS         = 3000;    // hard blink must be held this long
static const uint16_t UNLOCK_GAP_SAMPLES     = 31;      // breaks up to 0.12 s inside the hold are allowed
static const float    UNLOCK_KEEP_FRAC       = 0.80f;   // once holding, staying above 80 % of the level (24) is enough
static const float    UNLOCK_RELEASE_FRAC    = 0.5f;    // "released" = forehead signal below half the unlock level ...
static const uint16_t UNLOCK_RELEASE_SAMPLES = 64;      // ... for 0.25 s
static const uint32_t UNLOCK_GRACE_MS        = 1000;    // then 1 s before gestures count (eye opening ignored)
static const uint32_t ACTIVE_TIMEOUT_MS      = 3000;    // no command for 3 s -> LOCKED
static const float    UNLOCK_FACTORY_LEVEL   = UNLOCK_FACTORY_LEVEL_SET;   // untrained unlock level (forehead muscle signal)
static const float    UNLOCK_MIN_RATIO       = 2.0f;    // training: squeeze must be >= 2x the relaxed level
static const float    UNLOCK_MIN_LEVEL       = 30.0f;   // ... and at least this strong (trained level never below 30)
static const float    UNLOCK_REST_MULT       = 3.5f;    // unlock level >= 3.5x your relaxed forehead signal
static const float    UNLOCK_SQUEEZE_FRAC    = 0.7f;    // unlock level = 70 % of your sustained firm closure
static const float    UNLOCK_RATIO_SEP       = 1.6f;    // squeeze ratio >= 1.6x clench ratio -> ratio rule on
static const float    UNLOCK_RATIO_MARGIN    = 2.0f;    // ratio limit = 2x your clench's ratio (FINAL: was 1.3 - yawns passed)
static const float    UNLOCK_AVG_FRAC        = 0.35f;
static const float    SQ_HOLD_PCT            = 0.45f;   // FINAL: squeeze level = 45th percentile of the closure windows
static const float    BITE_AVG_FRAC          = 0.40f;
static const float    SLOW_RET_MAX           = -25.0f;  // FINAL: median blink return above this = slow blinker   // FINAL: bite-and-hold must average >= 40 % of your clench   // FINAL: 3 s hold must average >= 35 % of your calibration squeeze
static const float    UNLOCK_TEMPLE_SEP      = 2.5f;    // clench temple >= 2.5x squeeze temple -> temple ceiling
static const uint16_t SQ_REST_MAX            = 1024;    // training step 3 buffers (2 x 2 s relaxed)
static const float    LOCK_SMOOTH_ALPHA      = 1.0f / 38.0f;  // ~0.15 s extra smoothing for the hold logic
static const float    LOCK_HINT_FRAC         = 0.4f;    // feedback when a squeeze reaches 40 % of the unlock level
static const uint32_t LOCK_REMINDER_MS       = 10000;   // 'still LOCKED' reminder period
static const float    LOCK_CLOSE_EOG_FRAC    = 0.7f;    // eyelid-closing movement: eye signal >= 70 % of the blink trigger
static const uint32_t LOCK_CLOSE_BEFORE_MS   = 400;     // ... within 0.4 s before the hold starts (FINAL: was 0.8)
static const uint32_t LOCK_CLOSE_AFTER_MS    = 450;     // ... or within its first 0.45 s (FINAL: was 0.6)
static const float    LOCK_CLOSE_TRAIN_FRAC  = 0.5f;    // trained: needs half of your measured closure movement
static const float    LOCK_CLOSE_VS_REST     = 1.5f;    // trained: closure movement must be >= 1.5x the rest movement
static const float    LOCK_CLOSE_MIN         = 40.0f;   // ... and at least this big, else the eyelid check is off
static const float    LOCK_CLOSE_REST_MULT   = 1.3f;    // trained eyelid threshold >= 1.3x the rest movement
static const uint16_t SQ_SQUEEZE_MAX         = 1280;    // (2 x 2.5 s squeezed)
static const uint32_t SQ_LEAD_IN_MS          = 2500;    // pause before the step-3 countdown (clench settles)
static const uint8_t  CAP_TRACE_LEN          = PRE_TRIG_LEN + CAPTURE_POST_LEN;  // EMG samples per capture window

// =============================================================================
// 6. MOTOR
// =============================================================================
static const uint32_t MOTOR_PULSE_MS    = 420;
// Steering (double gestures). Windows are measured from the 1st gesture's decision;
// they match how soon this engine can see a 2nd blink (~0.4 s) / clench (~1.2 s).
static const uint32_t DOUBLE_BLINK_WINDOW_MS  = 1000;   // 2nd blink may come up to ~1 s after the 1st
static const uint32_t DOUBLE_CLENCH_WINDOW_MS = 1500;
static uint32_t dblClenchMs = DOUBLE_CLENCH_WINDOW_MS;     // FINAL: follows your bite length
static const uint16_t WATCH_RELEASE_SAMPLES   = 13;     // jaw relaxed (EMG < half threshold) for 50 ms
static const uint16_t WATCH_CLENCH_SAMPLES    = 20;     // 2nd clench: EMG above threshold for 78 ms in a row
static const uint16_t WATCH_RETURN_SAMPLES    = 80;     // 2nd blink must come back down within 0.31 s (FINAL: slow blinks)
// The 2nd blink of a double blink is usually softer than a normal blink, so it
// only needs this fraction of your trained blink peak / edge / return.
static const float    BLINK2_PEAK_FRAC        = 0.65f;
static const float    BLINK2_EDGE_FRAC        = 0.65f;
static const float    BLINK2_RETURN_FRAC      = 0.35f;
static const uint32_t SWALLOW_BLINK_MS        = 1000;   // engine's own decision on the 2nd blink is ignored
static const uint32_t SWALLOW_CLENCH_MS       = 1200;   // engine's own decision on the 2nd clench is ignored (PRO: was 2500)
static const uint32_t TURN_PULSE_DEFAULT_MS   = 300;    // ~90 deg spin on this chassis at full speed
static const uint32_t TURN_PULSE_MIN_MS       = 100;
static const uint32_t TURN_PULSE_MAX_MS       = 1000;
static const uint32_t TURN_PULSE_STEP_MS      = 50;
static const uint32_t SEQ_QUEUE_MAX_MS        = 1500;   // a command waiting for the motor is dropped after this
static const int16_t  PLOT_TURN_LEVEL         = 150;
static const uint32_t MOTOR_COOLDOWN_MS = 420;
static const int16_t  PLOT_MOTOR_LEVEL  = 300;    // plotter marker height for a motor pulse

// =============================================================================
// 7. SERIAL
// =============================================================================
static const uint32_t SERIAL_BAUD      = 115200;
static const size_t   TXQ_SIZE         = 4096;   // software TX queue (never blocks the sampler)
static const size_t   TXQ_RESERVE      = 768;    // kept free for event/command text
static const size_t   TX_LINE_MAX      = 256;
static const uint8_t  PLOT_DECIMATION  = 2;      // plotter at 128 Hz fits 115200 baud
static const uint32_t STATUS_PERIOD_MS = 1000;
static const uint8_t  RX_MAX_PER_LOOP  = 16;

static_assert((EMG_MA_LEN & (EMG_MA_LEN - 1)) == 0, "EMG_MA_LEN must be a power of two");
static_assert(TXQ_RESERVE + TX_LINE_MAX < TXQ_SIZE, "TX queue too small");

// =============================================================================
// TYPES
// =============================================================================
enum BiquadKind : uint8_t { BQ_LOWPASS, BQ_HIGHPASS, BQ_NOTCH };
enum SysMode : uint8_t { MODE_BOOT, MODE_CALIBRATE, MODE_WARMUP, MODE_RUN };
enum GestureState : uint8_t { ST_ARMED, ST_CAPTURE, ST_SETTLE };
enum Gesture : uint8_t { GEST_NONE, GEST_FORWARD, GEST_REVERSE, GEST_LEFT, GEST_RIGHT };
enum TrigSource : uint8_t { TRIG_POS, TRIG_NEG, TRIG_EMG, TRIG_DC };
enum MotorPhase : uint8_t { MOTOR_IDLE, MOTOR_PULSE, MOTOR_COOLDOWN };

// Direct Form II Transposed biquad (RBJ cookbook design, bilinear + prewarp).
struct Biquad {
  float b0, b1, b2, a1, a2;
  float z1, z2;

  void design(BiquadKind kind, double fs, double f0, double q) {
    const double w0 = 2.0 * DSP_PI * f0 / fs;
    const double cw = cos(w0);
    const double alpha = sin(w0) / (2.0 * q);
    const double a0 = 1.0 + alpha;
    double nb0 = 1.0, nb1 = -2.0 * cw, nb2 = 1.0;   // notch numerator
    switch (kind) {
      case BQ_LOWPASS:
        nb0 = (1.0 - cw) * 0.5; nb1 = 1.0 - cw; nb2 = nb0;
        break;
      case BQ_HIGHPASS:
        nb0 = (1.0 + cw) * 0.5; nb1 = -(1.0 + cw); nb2 = nb0;
        break;
      case BQ_NOTCH:
        break;
    }
    b0 = (float)(nb0 / a0);
    b1 = (float)(nb1 / a0);
    b2 = (float)(nb2 / a0);
    a1 = (float)(-2.0 * cw / a0);
    a2 = (float)((1.0 - alpha) / a0);
    reset();
  }

  void reset() { z1 = 0.0f; z2 = 0.0f; }

  inline float process(float x) {
    const float y = b0 * x + z1;
    z1 = b1 * x - a1 * y + z2;
    z2 = b2 * x - a2 * y;
    return y;
  }
};

// 5-sample running median (despike).
struct Median5 {
  float buf[MEDIAN_LEN];
  uint8_t idx;

  void reset() {
    for (uint8_t i = 0; i < MEDIAN_LEN; ++i) buf[i] = 0.0f;
    idx = 0;
  }

  inline float process(float x) {
    buf[idx] = x;
    idx = (uint8_t)((idx + 1) % MEDIAN_LEN);
    float s[MEDIAN_LEN];
    for (uint8_t i = 0; i < MEDIAN_LEN; ++i) s[i] = buf[i];
    for (uint8_t i = 1; i < MEDIAN_LEN; ++i) {          // insertion sort, 5 elements
      const float key = s[i];
      int8_t j = (int8_t)(i - 1);
      while (j >= 0 && s[j] > key) { s[j + 1] = s[j]; --j; }
      s[j + 1] = key;
    }
    return s[MEDIAN_LEN / 2];
  }
};

// 32-sample moving average; running sum re-summed every wrap to kill float drift.
struct MovingAverage32 {
  float buf[EMG_MA_LEN];
  float sum;
  uint8_t idx;

  void reset() {
    for (uint8_t i = 0; i < EMG_MA_LEN; ++i) buf[i] = 0.0f;
    sum = 0.0f;
    idx = 0;
  }

  inline float process(float x) {
    sum += x - buf[idx];
    buf[idx] = x;
    idx = (uint8_t)((idx + 1) & (EMG_MA_LEN - 1));
    if (idx == 0) {
      float exact = 0.0f;
      for (uint8_t i = 0; i < EMG_MA_LEN; ++i) exact += buf[i];
      sum = exact;
    }
    return sum * (1.0f / (float)EMG_MA_LEN);
  }
};

// Complete per-channel processing chain.
struct ChannelDsp {
  float dcOffset;
  Biquad notch;
  Median5 median;
  Biquad bp[4];
  Median5 rawMedian;   // EMG branch: pop detector reference
  Biquad emgNotch;     // EMG branch: its own 50 Hz notch (after pop rejection)
  Biquad emgHp[2];     // EMG branch: 4th-order Butterworth high-pass at 15 Hz
  MovingAverage32 emgMa;
  Biquad levelLp;
  float eog;     // 0.5-12 Hz EOG band
  float emg;     // EMG envelope
  float level;   // DC-coupled 2 Hz level (relative to calibrated offset)

  void design(double fs) {
    notch.design(BQ_NOTCH, fs, NOTCH_HZ, NOTCH_Q);
    bp[0].design(BQ_HIGHPASS, fs, BP_LOW_HZ, BUTTER4_Q1);
    bp[1].design(BQ_HIGHPASS, fs, BP_LOW_HZ, BUTTER4_Q2);
    bp[2].design(BQ_LOWPASS, fs, BP_HIGH_HZ, BUTTER4_Q1);
    bp[3].design(BQ_LOWPASS, fs, BP_HIGH_HZ, BUTTER4_Q2);
    emgNotch.design(BQ_NOTCH, fs, NOTCH_HZ, NOTCH_Q);
    // 4th order (same 15 Hz corner): the steeper skirt keeps large blink edges
    // (5-15 Hz content) out of the EMG envelope now that the median is bypassed.
    emgHp[0].design(BQ_HIGHPASS, fs, EMG_HP_HZ, BUTTER4_Q1);
    emgHp[1].design(BQ_HIGHPASS, fs, EMG_HP_HZ, BUTTER4_Q2);
    levelLp.design(BQ_LOWPASS, fs, LEVEL_LP_HZ, BUTTER2_Q);
    reset();
  }

  void reset() {
    notch.reset();
    median.reset();
    for (uint8_t i = 0; i < 4; ++i) bp[i].reset();
    rawMedian.reset();
    emgNotch.reset();
    emgHp[0].reset();
    emgHp[1].reset();
    emgMa.reset();
    levelLp.reset();
    eog = 0.0f;
    emg = 0.0f;
    level = 0.0f;
  }

  inline void process(float raw) {
    const float centered = raw - dcOffset;
    const float notched = notch.process(centered);
    const float despiked = median.process(notched);
    float band = despiked;
    for (uint8_t i = 0; i < 4; ++i) band = bp[i].process(band);
    eog = band;
    float emgIn;
    if (EMG_TAP_BEFORE_MEDIAN) {
      const float localMed = rawMedian.process(centered);
      const float popFree = (fabsf(centered - localMed) > EMG_POP_REJECT) ? localMed : centered;
      emgIn = emgNotch.process(popFree);
    } else {
      emgIn = despiked;
    }
    emg = emgMa.process(fabsf(emgHp[1].process(emgHp[0].process(emgIn))));
    level = levelLp.process(despiked);
  }
};

struct Thresholds {
  float trigV;
  float onsetRise;
  float emgTrigVMin;
  float fwdEmg;
  float fwdVMax;
  float fwdMinFloor;
  float revDeepMin;
  float biVMax;
  float biP2P;
  float biRetMin;
  float quietV;
  float quietEmg;
  float biteLevel;
  float biteLatchMin;
  float deepPeakMin;
};

// Base thresholds at scale x1.00. They start as the v13.2 constants ("factory")
// and are replaced by personal training. thrScale / biteScale multiply on top.
struct BaseThresholds {
  float trigV, onsetRise, emgTrigVMin, fwdEmg, fwdVMax, fwdMinFloor, revDeepMin;
  float biVMax, biP2P, biRetMin, quietV, quietEmg, biteLevel, biteLatchMin;
  float deepPeakMin;
};

enum TrainPhase : uint8_t { TRAIN_OFF, TRAIN_BLINK, TRAIN_CLENCH, TRAIN_SQUEEZE };
enum LockState : uint8_t { LOCK_LOCKED, LOCK_RELEASE, LOCK_ACTIVE };
// Which EMG the jaw rule uses. EMG_SRC_BOTH = max(forehead, temples) = v13.2.
enum EmgSource : uint8_t { EMG_SRC_BOTH = 0, EMG_SRC_TEMPLES = 1, EMG_SRC_FOREHEAD = 2 };

struct TrainSample {
  float vmax, vmin, p2p, ret, emg;
  uint8_t emgCount;
  uint8_t traceLen;
  float traceV[PRE_TRIG_LEN + CAPTURE_POST_LEN];  // forehead EMG envelope over the capture window
  float traceH[PRE_TRIG_LEN + CAPTURE_POST_LEN];  // temple EMG envelope over the capture window
};

struct SavedTraining {
  uint32_t magic;
  BaseThresholds base;
  uint8_t emgMinCount;
  uint8_t emgStrongCount;
  uint8_t emgSource;
  float unlockLevel;
  float unlockRatio;
  float unlockHMax;
  float closeEogNeed;
  uint8_t closeEogOff;
  uint8_t unlockBite;
  float biteLevel;
  uint16_t clenchDur;
  float holdAvg;
  float biteAvg;
  uint8_t retFree;
};

struct CaptureFeatures {
  // All V features are relative to the pre-onset baseline (cap.base). With a quiet
  // baseline (~0) they equal the raw values; on a decaying filter tail they keep
  // the morphology rules honest.
  float winVMax;      // max over the full window (pre-trigger + post-onset)
  float winVMin;      // min over the post-onset body (trigger sample onward)
  float preVMin;      // min over the pre-trigger samples
  float retMin;       // min of the samples after the running peak
  float winVEmgMax;   // max combined EMG envelope over the full window
  uint8_t emgAboveCount;  // window samples with combEmg > fwdEmg
  uint8_t ruleBits;   // bit0 FWD-EMG, bit1 FWD-shape, bit2 REV-deep, bit3 REV-biphasic
  bool conflict;      // both FORWARD and REVERSE rules matched
  float fwdScore;     // normalised evidence (only computed on a conflict)
  float revScore;
  float onsetRise;    // edge size measured inside the pre-trigger ring (absolute counts)
  float base;         // pre-onset V baseline: mean of the 2 oldest pre-trigger samples
  float baseLevel;    // DC-coupled level at the oldest pre-trigger sample
  uint16_t postCount;
  uint8_t preCount;
  TrigSource source;
};

// =============================================================================
// GLOBAL STATE
// =============================================================================
static ChannelDsp chH;
static ChannelDsp chV;

static Thresholds thr;
static const BaseThresholds FACTORY_BASE = {
  BASE_TRIG_V, BASE_ONSET_RISE, BASE_EMG_TRIG_VMIN, BASE_FWD_EMG, BASE_FWD_VMAX, BASE_FWD_MIN_FLOOR,
  BASE_REV_DEEP_MIN, BASE_BI_VMAX, BASE_BI_P2P, BASE_BI_RET_MIN, BASE_QUIET_V, BASE_QUIET_EMG,
  BASE_BITE_LEVEL, BASE_BITE_LATCH_MIN, BASE_DEEP_PEAK_MIN};
static BaseThresholds base = FACTORY_BASE;
static bool personalized = false;        // true = base came from personal training
static uint8_t emgMinCount = EMG_MIN_COUNT;        // jaw: EMG above threshold on >= this many window samples
static uint8_t emgStrongCount = EMG_STRONG_COUNT;  // conflict: this many = sustained bite, FORWARD wins
static EmgSource emgSource = EMG_SRC_BOTH;          // v13.2: max(forehead, temples)
static float preEmgV[PRE_TRIG_LEN];                  // per-pill EMG history (training only)
static float preEmgH[PRE_TRIG_LEN];
static float capTraceV[PRE_TRIG_LEN + CAPTURE_POST_LEN];  // per-pill EMG of the current capture window
static float capTraceH[PRE_TRIG_LEN + CAPTURE_POST_LEN];
static uint8_t capTraceLen = 0;
static uint8_t trainSavedMinCount = EMG_MIN_COUNT, trainSavedStrongCount = EMG_STRONG_COUNT;
static EmgSource trainSavedSource = EMG_SRC_BOTH;

static TrainPhase trainPhase = TRAIN_OFF;
static TrainSample trainBlink[TRAIN_SAMPLES];
static TrainSample trainClench[TRAIN_SAMPLES];
static uint8_t trainCount = 0;
static uint32_t trainStepStartMs = 0;
static BaseThresholds trainSavedBase;
static float trainSavedThrScale = SCALE_DEFAULT, trainSavedBiteScale = 1.0f;
static Preferences prefs;

// ---- FINAL ----
struct QTrack {                                  // running quantile (robust noise floor)
  float q;
  float p;
  uint32_t n;
  void add(float x) {
    if (n == 0) { q = x; n = 1; return; }
    const float s = ((n < NZ_FAST_SAMPLES) ? NZ_STEP_FAST : NZ_STEP_SLOW) * (fabsf(q) + 20.0f);
    q += (x > q) ? s * p : -s * (1.0f - p);
    if (q < 0.0f) q = 0.0f;
    ++n;
  }
};
static QTrack nzEog = {0.0f, 0.75f, 0};          // |eye signal|, 75th percentile
static QTrack nzEmgV = {0.0f, 0.50f, 0};         // forehead EMG, median
static QTrack nzEmgH = {0.0f, 0.50f, 0};         // temple EMG, median
static QTrack nzLvl = {0.0f, 0.90f, 0};
static float seqPeakBefore = 0.0f;               // peak of the blink that started the pending REVERSE
static float watchPeak = 0.0f, watchTrough = 0.0f;   // 2nd blink seen by the watcher
static float watchLow = 0.0f;                    // lowest point since the 1st blink's peak
static uint16_t watchJaw = 0;                    // jaw-EMG samples during the 2nd 'blink'
static bool settleAfterBlink = false;            // settling after a blink: a clench may start any moment
static uint16_t settleJaw = 0;          // slow eye-signal level wander (electrode drift), 90th pct
enum AcPhase : uint8_t { AC_OFF, AC_REST, AC_BLINK, AC_CLENCH, AC_SQUEEZE };
static AcPhase acPhase = AC_OFF;
static uint8_t acAttempt = 0;
static uint8_t acPrompts = 0, acMisses = 0;
static uint32_t acPhaseMs = 0, acPromptMs = 0, acNextPromptMs = 0, acReadyMs = 0, acCandMs = 0;
static bool acWinOpen = false, acBootPending = false, acHadSaved = false;
static bool acHaveCand = false;
static float acCandScore = 0.0f;
static TrainSample acCand;
static uint8_t acBlinkOk = 0, acClenchOk = 0;
static bool acSeparated = false;
static float acDur[TRAIN_SAMPLES];
static float acWinE[AC_WIN_MAX];
static uint16_t acWinN = 0;
static bool unlockBite = false;                  // unlock by bite-and-hold (eye squeeze too weak)
static float biteUnlockLevel = 0.0f;
static uint16_t clenchDurMs = 700;
static float unlockAvgNeed = 0.0f;               // FINAL: average forehead over the 3 s hold (from your squeeze)
static float holdSum = 0.0f;
static float biteAvgNeed = 0.0f;
static bool retFree = false;                     // FINAL: slow blinker - blink return not required                 // FINAL: average temple signal over a bite-and-hold
static uint32_t holdN = 0;               // your bite length (auto-calibration)
static uint32_t talkLongMs = 1700;               // one jaw burst this long = talking
static bool holdByBite = false;
static uint32_t releaseStartMs = 0;

// Safety lock state
static LockState lockState = LOCK_LOCKED;          // always starts LOCKED
static float unlockLevel = UNLOCK_FACTORY_LEVEL;   // forehead signal that counts as a hard blink
static float unlockRatio = UNLOCK_FACTORY_RATIO_SET;  // forehead/temple ratio rule (0 = off; set by training)
static float unlockHMax = 0.0f;                    // temple ceiling (0 = off; set by training)
static bool holdActive = false;
static uint32_t holdStartMs = 0;
static uint16_t holdGap = 0;
static uint8_t holdShownSec = 0;
static uint16_t releaseCount = 0;
static uint32_t lockGraceUntilMs = 0;
static uint32_t lastActivityMs = 0;
static float lockV = 0.0f, lockH = 0.0f;           // smoothed forehead / temple muscle signals
static uint32_t lastHintMs = 0, lastReminderMs = 0, partlySinceMs = 0;
static uint32_t lastEyeMoveMs = 0;                 // last eyelid-closing-like movement
static uint32_t lastMeterMs = 0;
static const uint16_t *buzzSeq = nullptr;
static uint8_t buzzLen = 0, buzzIdx = 0;
static uint32_t buzzStepMs = 0, buzzQuietUntilMs = 0;
static bool buzzOnNow = false, holdTicked = false;
static bool closureSeen = false, noCloseHinted = false;
static float closeEogNeed = 0.0f;                  // trained eyelid threshold (0 = use factory rule)
static bool closeEogOff = false;                   // trained: eyelid check switched off
static float sqClose1 = 0.0f, sqClose2 = 0.0f, sqRestEog = 0.0f;   // training step 3 eye-signal peaks
// Training step 3 (unlock squeeze)
static uint32_t sqStartMs = 0;
static uint8_t sqLastSec = 0xFF;
static float sqRestV[SQ_REST_MAX], sqRestH[SQ_REST_MAX];
static float sqSqzV[SQ_SQUEEZE_MAX], sqSqzH[SQ_SQUEEZE_MAX], sqSqzT[SQ_SQUEEZE_MAX];
static uint16_t sqRestN = 0, sqSqueezeN = 0;
static float thrScale = SCALE_DEFAULT;   // global amplitude scale (1/2/3, +/-, 0)
static float biteScale = 1.0f;           // FORWARD-only sensitivity (j/k)
static float combEmg = 0.0f;             // max(vertical EMG, temple EMG) this sample

static SysMode sysMode = MODE_BOOT;
static uint16_t modeCounter = 0;

static double calSumH = 0.0, calSumV = 0.0, calSqH = 0.0, calSqV = 0.0;

static GestureState gState = ST_SETTLE;
static float preV[PRE_TRIG_LEN];
static float preEmg[PRE_TRIG_LEN];
static float preLevel[PRE_TRIG_LEN];
static uint8_t preHead = 0;
static uint8_t preCount = 0;
static CaptureFeatures cap;
static uint32_t settleSamples = 0;
static uint8_t quietRun = 0;
static bool slowEdgeNoted = false;
static bool biteLatched = false;
static float biteBase = 0.0f;         // DC level before the bite
static float biteRise = 0.0f;         // peak DC shift while held
static float levelBase = 0.0f;        // slow baseline of the DC-coupled level
static uint16_t holdoffSamples = 0;   // extra re-arm delay (bite release, ADC clipping)

static bool vOnRail = false;
static bool satV = false, satH = false;
static uint16_t satRunV = 0, clearRunV = 0, satRunH = 0, clearRunH = 0;

static uint32_t sampleIndex = 0;
static uint32_t fwdCount = 0, revCount = 0, ignoredCount = 0, busyDropCount = 0, slowEdgeCount = 0;
static uint32_t leftCount = 0, rightCount = 0;
// Steering sequencer
static Gesture seqPending = GEST_NONE;     // 1st gesture waiting for a possible 2nd one
static uint32_t seqPendingMs = 0;
static Gesture seqQueued = GEST_NONE;      // decided command waiting for the motor to be free
static uint32_t seqQueuedMs = 0;
static uint32_t turnPulseMs = TURN_PULSE_DEFAULT_MS;
static uint32_t motorPulseMs = MOTOR_PULSE_MS;   // length of the current pulse
// 2nd-gesture watcher (runs on every sample while a gesture is pending)
static uint8_t watchPhase = 0;             // 0 = 1st gesture fading, 1 = looking for the 2nd, 2 = 2nd blink must return
static uint16_t watchCount = 0;
static float watchV[PRE_TRIG_LEN];         // last 39 ms of V, for the sharp-edge test
static uint8_t watchHead = 0;
static Gesture seqSwallow = GEST_NONE;     // the engine's own capture of a 2nd gesture is ignored ...
static uint32_t seqSwallowUntilMs = 0;     // ... until this time

static MotorPhase motorPhase = MOTOR_IDLE;
static char motorCmd = 'F';              // command of the current pulse ('F' or 'R')
static uint32_t motorPhaseStartMs = 0;
static int16_t motorMarker = 0;

// ESP-NOW radio state
static bool radioReady = false;
static char radioCmd = 'S';                // command currently on air
static uint8_t radioBurst = 0;             // copies of a NEW command still to send
static uint32_t radioChangeMs = 0;         // when the current command was requested
static uint8_t radioSeq = 0;
static uint32_t radioLastSendMs = 0;
static uint32_t radioTxOk = 0, radioTxFail = 0;

static uint32_t nextTickUs = 0;
static uint32_t tickFracAcc = 0;
static uint32_t overrunCount = 0;
static uint32_t lastTickStartUs = 0;  // micros() at the start of the latest tick
static uint32_t fsRefTickUs = 0;      // tick timestamp at the previous status line
static uint32_t fsRefIndex = 0;
static bool fsRefValid = false;
static uint64_t fsTotalUs = 0;        // long-run rate = ticks / time between tick starts
static uint64_t fsTotalTicks = 0;
static uint32_t statMaxBusyUs = 0;
static uint32_t statMaxLateUs = 0;
static uint32_t lastStatusMs = 0;

static bool plotterMode = false;
static uint8_t plotDecim = 0;

static uint8_t txq[TXQ_SIZE];
static size_t txHead = 0, txTail = 0, txCount = 0;
static uint32_t txDroppedBytes = 0;

// =============================================================================
// PROTOTYPES
// =============================================================================
static bool txEnqueue(const char *data, size_t len, bool essential);
static void txVPrintf(bool essential, const char *fmt, va_list args);
static void txPrintf(bool essential, const char *fmt, ...) __attribute__((format(printf, 2, 3)));
static void logDebug(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
static void txService();
static float readAdc(uint8_t pin);
static void applyScales();
static void trainStart();
static void trainAbort(const char *why);
static void trainRecord(float p2p);
static void trainFinish();
static void trainService();
static bool trainLoad();
static void trainSave();
static void trainForget();
static float clampScale(float v, float lo, float hi);
static void setThrScale(float s);
static void setBiteScale(float b);
static void printThresholds(const char *label);
static void printHelp();
static const char *modeName(SysMode m);
static const char *stateName(GestureState s);
static const char *motorName();
static void radioInit();
static void radioSend(char cmd);
static bool radioTransmit(char cmd);
static void radioService();
static const char *resetReasonName(esp_reset_reason_t r);
static bool motorStart(Gesture g);
static const char *gestureName(Gesture g);
static void seqFire(Gesture g);
static const char *seqGesture(Gesture g);
static void seqService();
static void seqClear();
static void seqWatch(float v, float emg);
static const char *lockName();
static void lockEngage(const char *why);
static void lockActivate(const char *why);
static bool lockAllowsCommands();
static void lockNoteActivity();
static void lockSmooth(float vEmg, float hEmg);
static void buzzInit();
static void buzz(const uint16_t *seq, uint8_t len);
static void buzzService();
static bool buzzArtifact();
static void lockEyeMove(float vEog);
static void lockSample(float vEmg, float hEmg);
static void lockService();
static void lockToggleByKey();
static void squeezeStart();
static void squeezeSample(float vEmg, float hEmg);
static void squeezeService();
static float median5(const float *v);
static void applyNoiseFloors();
static float effUnlockLevel();
static float effBiteLevel();
static void applyClenchTiming();
static void noiseTrack(float vEog, float vEmg, float hEmg);
static void autoCalStart(const char *why);
static void acSample(float vEmg, float hEmg);
static void acCapture(float p2p);
static void acComplete(bool squeezeOk, bool failedStep);
static void trainForgetSoft();
static void buttonsInit();
static void buttonsService();
static float jawNoise();
static float glanceLimit();
static bool blinkPairUneven(float first, float second);
static void slowRingPush(float v);
static float slowRingMin();
static bool slowBlinkOnset(float v);
static bool dipRiseOnset(float v);
static float firmP2P(float frac);
static float firmPeak(float frac);
static void rightDefer_clear();
static Gesture talkFilter(Gesture g);
static void talkDeferRight();
static bool glanceVeto(Gesture g);
static bool softBlinkVeto(Gesture g);
static bool talkBusyNow();
static float seqLevelBefore = 0.0f;           // PRO: eye-signal level before the pending blink
static float seqP2PBefore = 0.0f;             // PRO: size of the blink that started the pending command
static float gestP2P = 0.0f;                  // PRO: size of the last detected gesture
static bool seqJawBusy = false;               // PRO: jaw was already moving before the pending clench
static void setTurnPulse(uint32_t ms);
static void loadTurnPulse();
static void motorService();
static void motorStopAll();
static void preTriggerClear();
static void preTriggerPush(float v, float emg, float level);
static void enterSettle();
static void latchBite(float base, float rise);
static void captureAdd(float v, float emg);
static void captureStart(float v, float emg, TrigSource src, float rise);
static void finishCapture(Gesture g, const char *why);
static void captureEvaluate();
static void settleStep(float v, float emg, float level);
static void gestureStep(float v, float emg, float level);
static void updateSaturation(float rawH, float rawV);
static void beginCalibration(bool requestedByUser);
static void calibrationStep(float rawH, float rawV);
static void finishCalibration();
static void emitPlotLine();
static void onSampleTick();
static void advanceTick();
static void manualPulse(Gesture g);
static void stopAndRearm();
static void handleHotkey(char c);
static void serviceSerialInput();
static void housekeeping();

// =============================================================================
// NON-BLOCKING SERIAL OUTPUT
// Text is queued in RAM and drained only as fast as the port accepts it
// (Serial.write is never asked for more than availableForWrite()), so a slow
// or disconnected host can never stall the 256 Hz sampler.
// =============================================================================
static bool txEnqueue(const char *data, size_t len, bool essential) {
  if (len == 0) return true;
  const size_t freeBytes = TXQ_SIZE - txCount;
  const size_t required = essential ? len : (len + TXQ_RESERVE);
  if (required > freeBytes) {
    txDroppedBytes += (uint32_t)len;
    return false;
  }
  const size_t firstPart = (len < (TXQ_SIZE - txHead)) ? len : (TXQ_SIZE - txHead);
  memcpy(&txq[txHead], data, firstPart);
  if (len > firstPart) memcpy(&txq[0], data + firstPart, len - firstPart);
  txHead = (txHead + len) % TXQ_SIZE;
  txCount += len;
  return true;
}

static void txVPrintf(bool essential, const char *fmt, va_list args) {
  char line[TX_LINE_MAX];
  const int n = vsnprintf(line, sizeof(line), fmt, args);
  if (n <= 0) return;
  size_t len = (size_t)n;
  if (len >= sizeof(line)) {           // truncated: keep the line terminated
    len = sizeof(line) - 1;
    line[len - 1] = '\n';
  }
  txEnqueue(line, len, essential);
}

static void txPrintf(bool essential, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  txVPrintf(essential, fmt, args);
  va_end(args);
}

// Human-readable event text: debug mode only (keeps the plotter stream clean).
static void logDebug(const char *fmt, ...) {
  if (plotterMode) return;
  va_list args;
  va_start(args, fmt);
  txVPrintf(true, fmt, args);
  va_end(args);
}

static void txService() {
  if (txCount == 0) return;
  const int room = Serial.availableForWrite();
  if (room <= 0) return;
  size_t chunk = (size_t)room;
  if (chunk > txCount) chunk = txCount;
  if (chunk > TXQ_SIZE - txTail) chunk = TXQ_SIZE - txTail;
  const size_t written = Serial.write(&txq[txTail], chunk);
  if (written > chunk) return;          // defensive: never trust a bogus return value
  txTail = (txTail + written) % TXQ_SIZE;
  txCount -= written;
}

// =============================================================================
// ADC
// =============================================================================
static float readAdc(uint8_t pin) {
  uint32_t acc = 0;
  for (uint8_t i = 0; i < ADC_OVERSAMPLE; ++i) acc += (uint32_t)analogRead(pin);
  return (float)acc / (float)ADC_OVERSAMPLE;
}

// =============================================================================
// THRESHOLDS / UI TEXT
// =============================================================================
// Recomputes every threshold from thrScale (global) and biteScale (FORWARD + re-arm EMG).
static void applyScales() {
  const float s = thrScale;
  const float b = biteScale;
  thr.trigV        = base.trigV * s;
  thr.onsetRise    = base.onsetRise * s;
  thr.emgTrigVMin  = base.emgTrigVMin * s;
  thr.quietV       = base.quietV * s;
  thr.biteLevel    = base.biteLevel * s;
  thr.biteLatchMin = base.biteLatchMin * s;
  // FORWARD (jaw clench): biteScale < 1 = more sensitive.
  thr.fwdVMax      = base.fwdVMax * s * b;
  thr.fwdMinFloor  = base.fwdMinFloor * s / b;
  thr.fwdEmg       = base.fwdEmg * s * b;
  // The re-arm EMG gate is "no bite in progress", so it moves with the bite
  // threshold: the 65/70 hysteresis is preserved at every biteScale.
  thr.quietEmg     = base.quietEmg * s * b;
  // REVERSE (blink): global scale only.
  thr.revDeepMin   = base.revDeepMin * s;
  thr.biVMax       = base.biVMax * s;
  thr.biP2P        = base.biP2P * s;
  thr.biRetMin     = base.biRetMin * s;
  thr.deepPeakMin  = base.deepPeakMin * s;
  applyNoiseFloors();                            // FINAL: never below the measured resting noise
}

static float clampScale(float v, float lo, float hi) {
  v = roundf(v * 100.0f) / 100.0f;
  if (v < lo) v = lo;
  if (v > hi) v = hi;
  return v;
}

static void setThrScale(float s) {
  thrScale = clampScale(s, SCALE_MIN, SCALE_MAX);
  applyScales();
}

static void setBiteScale(float b) {
  biteScale = clampScale(b, BITE_SCALE_MIN, BITE_SCALE_MAX);
  applyScales();
}

static void printThresholds(const char *label) {
  const char *limS = (thrScale <= SCALE_MIN || thrScale >= SCALE_MAX) ? " (limit)" : "";
  const char *limB = (biteScale <= BITE_SCALE_MIN || biteScale >= BITE_SCALE_MAX) ? " (limit)" : "";
  txPrintf(true, "[CMD] %s -> %s thresholds | threshold scale x%.2f%s | bite scale x%.2f%s\n", label,
           personalized ? "PERSONAL (trained)" : "factory v13.2", (double)thrScale, limS, (double)biteScale, limB);
  txPrintf(true, "      trigger |V|>=%.0f (edge>=%.0f in 39 ms) | quiet: |V|<%.0f & EMG<%.0f\n",
           (double)thr.trigV, (double)thr.onsetRise, (double)thr.quietV, (double)thr.quietEmg);
  if (base.fwdVMax >= RULE_OFF) {
    txPrintf(true, "      FWD: EMG>%.0f on >=%u samples | shape rule off\n", (double)thr.fwdEmg, (unsigned)emgMinCount);
  } else {
    txPrintf(true, "      FWD: EMG>%.0f on >=%u samples | or winVMax>=%+.0f & winVMin>=%+.0f & max>%.1fx|dip|\n",
             (double)thr.fwdEmg, (unsigned)emgMinCount, (double)thr.fwdVMax, (double)thr.fwdMinFloor,
             (double)FWD_DOMINANCE);
  }
  if (base.deepPeakMin > -RULE_OFF) {
    txPrintf(true, "      REV: winVMin<=%+.0f & winVMax>=%+.0f | or winVMax>=%+.0f & P2P>=%.0f & return<=%+.0f | both: FWD if EMG>=%u samples\n",
             (double)thr.revDeepMin, (double)thr.deepPeakMin, (double)thr.biVMax, (double)thr.biP2P,
             (double)thr.biRetMin, (unsigned)emgStrongCount);
  } else {
    txPrintf(true, "      REV: winVMin<=%+.0f | or winVMax>=%+.0f & P2P>=%.0f & return<=%+.0f | both: FWD if EMG>=%u samples\n",
             (double)thr.revDeepMin, (double)thr.biVMax, (double)thr.biP2P, (double)thr.biRetMin,
             (unsigned)emgStrongCount);
  }
}

static void printHelp() {
  txPrintf(true, "\n============ EOG/EMG BCI -> RC CAR (ESP32-S3) firmware v13.2-ESPNOW-r3-LR2-FINAL\n");
  txPrintf(true, " Fs 256.000 Hz | shape: PIN_VERT GPIO%u | EMG: max(GPIO%u, temples GPIO%u)\n",
           (unsigned)PIN_VERT, (unsigned)PIN_VERT, (unsigned)PIN_HORIZ);
  txPrintf(true, " JAW CLENCH -> FORWARD 'F' | EYE BLINK -> REVERSE 'R' | idle 'S' (ESP-NOW broadcast, ch %u)\n",
           (unsigned)ESPNOW_CHANNEL);
  txPrintf(true, " Pulse %lu ms + cooldown %lu ms | heartbeat every %lu ms\n",
           (unsigned long)MOTOR_PULSE_MS, (unsigned long)MOTOR_COOLDOWN_MS, (unsigned long)HEARTBEAT_MS);
  txPrintf(true, " Keys: 1/2/3 preset x0.85/x1.00/x1.35 | +/- all thresholds +/-10%% | 0 reset x1.00\n");
  txPrintf(true, "       j/k jaw clench FORWARD 10%% more / less sensitive (blink unaffected)\n");
  txPrintf(true, "       f/b test pulse FWD/REV | s STOP+clear+re-arm | c recalibrate (sit still 2 s)\n");
  txPrintf(true, "       t / a calibrate again (follow the beeps) | y old keyboard-paced training | u back to factory v13.2 thresholds\n");
  txPrintf(true, " AUTO : calibrates by itself at power-on | NEW button GPIO7 or hold BOOT 1 s = calibrate again | STOP button GPIO15\n");
  txPrintf(true, " LOCK : starts LOCKED | close your eyes firmly 3 s = unlock | 3 s idle = lock | o = lock/unlock | z = buzzer test (GPIO13)\n");
  txPrintf(true, " STEER: blink-blink = TURN LEFT | bite-relax-bite = TURN RIGHT | l/r test turn | [ ] turn time (%lu ms)\n",
           (unsigned long)turnPulseMs);
  txPrintf(true, "       p plotter CSV <-> debug text | h help\n");
  txPrintf(true, "=======================================================================\n");
}

static const char *modeName(SysMode m) {
  switch (m) {
    case MODE_BOOT:      return "BOOT";
    case MODE_CALIBRATE: return "CALIBRATE";
    case MODE_WARMUP:    return "WARMUP";
    case MODE_RUN:       return "RUN";
  }
  return "?";
}

static const char *stateName(GestureState s) {
  switch (s) {
    case ST_ARMED:   return "ARMED";
    case ST_CAPTURE: return "CAPTURE";
    case ST_SETTLE:  return "SETTLE";
  }
  return "?";
}

static const char *motorName() {
  switch (motorPhase) {
    case MOTOR_IDLE:     return "IDLE";
    case MOTOR_PULSE:    return (motorCmd == 'F') ? "FWD" : (motorCmd == 'R') ? "REV" : (motorCmd == '<') ? "LEFT" : "RIGHT";
    case MOTOR_COOLDOWN: return "COOLDOWN";
  }
  return "?";
}

// =============================================================================
// ESP-NOW RADIO
// No send callback is registered (its signature differs between core 2.x and
// 3.x); esp_now_send()'s return value is enough for the status counters.
//
// radioSend() only REQUESTS a command. The packets go out from radioService(),
// which loop() calls immediately after each 256 Hz sample tick, so every Wi-Fi
// burst happens just after the ADC reads and is long finished (< 1 ms) before
// the next sample 3.9 ms later.
// =============================================================================
static void radioInit() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_ps(WIFI_PS_NONE);                          // lowest latency
  if (esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE) != ESP_OK) {
    txPrintf(true, "[RADIO] WARNING: could not set Wi-Fi channel %u - car may not hear this board\n",
             (unsigned)ESPNOW_CHANNEL);
  }
  if (esp_wifi_set_max_tx_power(RADIO_TX_POWER_QDBM) != ESP_OK) {
    txPrintf(true, "[RADIO] note: TX power not changed (using default)\n");
  }

  if (esp_now_init() != ESP_OK) {
    txPrintf(true, "[RADIO] ERROR: esp_now_init failed - car will not receive commands\n");
    return;
  }
  esp_now_peer_info_t peer;
  memset(&peer, 0, sizeof(peer));
  memcpy(peer.peer_addr, BROADCAST_MAC, 6);
  peer.channel = ESPNOW_CHANNEL;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;
  if (esp_now_add_peer(&peer) != ESP_OK) {
    txPrintf(true, "[RADIO] ERROR: could not add broadcast peer\n");
    return;
  }
  radioReady = true;
  int8_t txPower = 0;
  esp_wifi_get_max_tx_power(&txPower);
  txPrintf(true, "[RADIO] ESP-NOW ready: broadcasting on channel %u, TX power %.1f dBm (sender MAC %s)\n",
           (unsigned)ESPNOW_CHANNEL, (double)txPower * 0.25, WiFi.macAddress().c_str());
  radioSend('S');
  radioLastSendMs = (uint32_t)millis();
}

// Requests a command: it becomes the heartbeat command, and it is queued to go
// out (1 + RADIO_REPEATS) times on the next ticks. Called from the gesture
// engine (inside a tick) and from motorService() / hotkeys (between ticks).
static void radioSend(char cmd) {
  radioCmd = cmd;
  radioBurst = (uint8_t)(1 + RADIO_REPEATS);
  radioChangeMs = (uint32_t)millis();
}

static bool radioTransmit(char cmd) {
  CarPacket pkt;
  pkt.magic0 = PKT_MAGIC0;
  pkt.magic1 = PKT_MAGIC1;
  pkt.cmd = (uint8_t)cmd;
  pkt.seq = radioSeq;
  if (esp_now_send(BROADCAST_MAC, (const uint8_t *)&pkt, sizeof(pkt)) != ESP_OK) {
    ++radioTxFail;
    return false;
  }
  ++radioSeq;
  ++radioTxOk;
  return true;
}

// Called by loop() immediately after every sample tick (see note above).
// At most one packet per tick: a burst copy if one is pending, otherwise the
// heartbeat (25 ms for 250 ms after a change, then 80 ms).
// A failed send is simply retried on the next tick (~4 ms).
static void radioService() {
  if (!radioReady) return;
  const uint32_t now = (uint32_t)millis();
  const uint32_t period = ((uint32_t)(now - radioChangeMs) < FAST_HEARTBEAT_SPAN) ? FAST_HEARTBEAT_MS : HEARTBEAT_MS;
  const bool heartbeatDue = (uint32_t)(now - radioLastSendMs) >= period;
  if (radioBurst == 0 && !heartbeatDue) return;
  if (radioTransmit(radioCmd)) {
    if (radioBurst > 0) --radioBurst;
    radioLastSendMs = now;
  }
}

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

// =============================================================================
// MOTOR (non-blocking micro-pulses) - same timing as v13.2, sent over ESP-NOW
//   v13.2 pin HIGH -> 'F' / 'R'      v13.2 pin LOW -> 'S'
// =============================================================================
static bool motorStart(Gesture g) {
  if (g == GEST_NONE) return false;
  if (motorPhase != MOTOR_IDLE) return false;
  motorCmd = (g == GEST_FORWARD) ? 'F' : (g == GEST_REVERSE) ? 'R' : (g == GEST_LEFT) ? '<' : '>';
  motorPulseMs = (g == GEST_LEFT || g == GEST_RIGHT) ? turnPulseMs : MOTOR_PULSE_MS;
  radioSend(motorCmd);
  motorMarker = (g == GEST_FORWARD)   ? PLOT_MOTOR_LEVEL
                : (g == GEST_REVERSE) ? (int16_t)-PLOT_MOTOR_LEVEL
                : (g == GEST_LEFT)    ? (int16_t)-PLOT_TURN_LEVEL
                                      : PLOT_TURN_LEVEL;
  motorPhase = MOTOR_PULSE;
  motorPhaseStartMs = (uint32_t)millis();
  return true;
}

static void motorService() {
  if (motorPhase == MOTOR_IDLE) return;
  const uint32_t now = (uint32_t)millis();
  const uint32_t elapsed = now - motorPhaseStartMs;
  if (motorPhase == MOTOR_PULSE) {
    if (elapsed >= motorPulseMs) {
      radioSend('S');
      motorMarker = 0;
      motorPhase = MOTOR_COOLDOWN;
      motorPhaseStartMs = now;
    }
  } else if (elapsed >= MOTOR_COOLDOWN_MS) {
    motorPhase = MOTOR_IDLE;
  }
}

static void motorStopAll() {
  radioSend('S');
  motorPhase = MOTOR_IDLE;
  motorMarker = 0;
  seqClear();
}

// =============================================================================
// STEERING SEQUENCER (double blink = LEFT, double clench = RIGHT)
// Runs AFTER the gesture engine has decided FORWARD / REVERSE, so blink /
// clench detection (and personal training) are exactly as before. A single
// gesture waits for its window; a 2nd identical gesture inside it is a turn.
// =============================================================================
static const char *gestureName(Gesture g) {
  switch (g) {
    case GEST_FORWARD: return "FORWARD";
    case GEST_REVERSE: return "REVERSE";
    case GEST_LEFT:    return "TURN LEFT";
    case GEST_RIGHT:   return "TURN RIGHT";
    default:           return "none";
  }
}

static void seqFire(Gesture g) {
  if (lockState != LOCK_ACTIVE) return;          // safety: nothing moves unless ACTIVE
  g = talkFilter(g);                             // PRO: talking / chewing is not a clench gesture
  if (g == GEST_NONE) return;
  if (glanceVeto(g)) return;                     // PRO: looking down/up is not a blink
  if (softBlinkVeto(g)) return;                  // PRO: an everyday blink is not a command (after training)
  lockNoteActivity();                            // a real command keeps the car unlocked
  if (motorStart(g)) {
    if (g == GEST_FORWARD) ++fwdCount;
    else if (g == GEST_REVERSE) ++revCount;
    else if (g == GEST_LEFT) ++leftCount;
    else ++rightCount;
    logDebug("[SEQ  %8.2fs] -> %s\n", (double)millis() / 1000.0, gestureName(g));
  } else {                                       // motor still busy: send as soon as it is free
    seqQueued = g;
    seqQueuedMs = (uint32_t)millis();
  }
}

// Called with each FORWARD / REVERSE decision. Returns the text for the [GEST] line.
static const char *seqGesture(Gesture g) {
  const uint32_t now = (uint32_t)millis();
  if (trainPhase != TRAIN_OFF) return "training - ignored";
  if (lockState == LOCK_LOCKED) return "LOCKED - ignored (hold hard blink 3 s)";
  if (!lockAllowsCommands()) return "just unlocked - ignored";
  lockNoteActivity();
  if (g == seqSwallow && (int32_t)(seqSwallowUntilMs - now) > 0) {
    return "(2nd gesture of the turn - already done)";
  }
  if (seqPending == g && g == GEST_REVERSE && personalized &&
      gestP2P < firmP2P(BLINK2_FIRM_FRAC)) {
    return "soft 2nd blink (everyday blink) - ignored";   // FINAL: REVERSE stays REVERSE
  }
  if (seqPending == g && g == GEST_REVERSE && personalized && blinkPairUneven(seqP2PBefore, gestP2P)) {
    // FINAL: a deliberate double blink is two similar blinks. A clearly smaller 1st one
    // was an everyday blink: THIS blink starts the command (REVERSE, or LEFT if another follows)
    seqPendingMs = now;
    seqLevelBefore = cap.baseLevel;
    seqP2PBefore = gestP2P;
    seqPeakBefore = cap.winVMax;
    watchPhase = 0;
    watchCount = 0;
    return "1st blink was an everyday blink - this one starts the command";
  }
  if (seqPending == g) {                         // 2nd identical gesture inside the window
    seqPending = GEST_NONE;
    if (g == GEST_REVERSE) seqFire(GEST_LEFT);
    else talkDeferRight();                       // PRO: RIGHT after 0.9 s if no 3rd bite
    return (g == GEST_REVERSE) ? "DOUBLE BLINK -> TURN LEFT" : "DOUBLE CLENCH -> TURN RIGHT";
  }
  if (seqPending != GEST_NONE) seqFire(seqPending);   // a different gesture: do the waiting one now
  seqPending = g;
  seqPendingMs = now;
  seqLevelBefore = cap.baseLevel;               // PRO: for the look-down check
  seqP2PBefore = gestP2P;                        // PRO: for the soft-blink check
  seqPeakBefore = cap.winVMax;                   // FINAL: for the pair check
  seqJawBusy = talkBusyNow();                    // PRO: jaw already moving = talking
  watchPhase = 0;
  watchCount = 0;
  return (g == GEST_REVERSE) ? "wait: blink again = LEFT" : "wait: clench again = RIGHT";
}

static void seqService() {
  const uint32_t now = (uint32_t)millis();
  if (seqQueued != GEST_NONE) {
    if (motorPhase == MOTOR_IDLE) {
      const Gesture g = seqQueued;
      seqQueued = GEST_NONE;
      seqFire(g);
    } else if ((uint32_t)(now - seqQueuedMs) >= SEQ_QUEUE_MAX_MS) {
      seqQueued = GEST_NONE;
      ++busyDropCount;
    }
  }
  if (seqPending == GEST_NONE) return;
  const uint32_t window = (seqPending == GEST_FORWARD) ? dblClenchMs : DOUBLE_BLINK_WINDOW_MS;
  if ((uint32_t)(now - seqPendingMs) >= window) {
    // FINAL: a slow 2nd blink may still be under way - wait for it (max 0.6 s more)
    if (seqPending == GEST_REVERSE && (uint32_t)(now - seqPendingMs) < window + BLINK_EXTEND_MS &&
        (gState == ST_CAPTURE || watchPhase >= 2 || chV.eog > 0.5f * thr.biVMax)) return;
    const Gesture g = seqPending;
    seqPending = GEST_NONE;
    seqFire(g);                                  // no 2nd gesture: plain FORWARD / REVERSE
  }
}

static void seqClear() {
  rightDefer_clear();
  seqPending = GEST_NONE;
  seqQueued = GEST_NONE;
  seqSwallow = GEST_NONE;
}

// Called on every sample in RUN mode. While a blink is pending it looks for a
// 2nd sharp blink peak; while a clench is pending it looks for the jaw to relax
// and then bite again. Works inside the engine's refractory / bite-hold time.
static float blink2Need() {
  const float base2 = BLINK2_PEAK_FRAC * thr.biVMax;
  if (!personalized) return base2;
  const float firm = firmPeak(BLINK2_FIRM_FRAC);  // FINAL: % of your blink peak, minus room for eye noise
  return (firm > base2) ? firm : base2;
}

static void seqWatch(float v, float emg) {
  watchV[watchHead] = v;
  watchHead = (uint8_t)((watchHead + 1) % PRE_TRIG_LEN);
  if (seqPending == GEST_NONE || trainPhase != TRAIN_OFF || !lockAllowsCommands()) return;
  if (satV || vOnRail) {                         // electrode off / clipped: never invent a 2nd gesture
    watchPhase = 0;
    watchCount = 0;
    return;
  }

  bool second = false;
  if (seqPending == GEST_REVERSE) {
    if (watchPhase == 0) {
      if (v < 0.5f * thr.biVMax) { watchPhase = 1; watchLow = v; }   // the 1st blink's peak is over
    } else if (watchPhase == 1) {
      float vMin = watchV[0];
      for (uint8_t i = 1; i < PRE_TRIG_LEN; ++i) if (watchV[i] < vMin) vMin = watchV[i];
      if (v < watchLow) watchLow = v;
      // a new peak: sharp and high enough - or (FINAL) a blink that starts in the 1st
      // blink's dip: it peaks lower, so its rise from that dip is measured instead
      if (((v >= blink2Need()) && (v - vMin >= BLINK2_EDGE_FRAC * thr.onsetRise)) ||
          ((v - watchLow >= BLINK2_RISE_K * blink2Need()) && (v >= 0.5f * blink2Need()))) {
        watchPhase = 2;
        watchCount = 0;
        watchPeak = v;
        watchTrough = v;
        watchJaw = 0;
      }
    } else if (watchPhase >= 2 && emg > thr.fwdEmg && ++watchJaw >= WATCH_JAW_MAX) {
      watchPhase = 0;                                      // FINAL: jaw muscle working = a clench, not a blink
      watchCount = 0;
    } else if (watchPhase == 2) {
      // ... is a blink only if it comes straight back down. Looking up, or the
      // forehead rise of a clench, stays up and is not counted.
      if (v > watchPeak) watchPeak = v;
      if (v < watchTrough) watchTrough = v;
      if (v <= BLINK2_RETURN_FRAC * thr.biRetMin) {
        watchPhase = 3;                                    // FINAL: follow its dip to measure its size
        watchCount = 0;
      } else if (++watchCount >= WATCH_RETURN_SAMPLES) {
        watchPhase = 0;                                    // not a blink: wait for the next one
      }
    } else {
      if (v < watchTrough) watchTrough = v;
      // follow the dip to its bottom (a slow blink's dip comes ~0.2 s after its peak)
      if (++watchCount >= WATCH_SIZE_SAMPLES || (watchCount >= 8 && v > watchTrough + 25.0f)) {
        // FINAL: the same 'soft 2nd blink' rule as the engine uses: an everyday blink is not a pair
        if (personalized && (watchPeak - watchTrough) < firmP2P(BLINK2_FIRM_FRAC)) {
          watchPhase = 0;
          watchCount = 0;
          logDebug("[SEQ  %8.2fs] soft 2nd blink (size %.0f) - not a double blink\n", (double)millis() / 1000.0,
                   (double)(watchPeak - watchTrough));
        } else {
          second = true;
        }
      }
    }
  } else if (seqPending == GEST_FORWARD) {
    if (watchPhase == 0) {
      if (emg < 0.5f * thr.fwdEmg) {                       // jaw relaxed after the 1st clench
        if (++watchCount >= WATCH_RELEASE_SAMPLES) { watchPhase = 1; watchCount = 0; }
      } else {
        watchCount = 0;
      }
    } else {
      if (emg > thr.fwdEmg) {                              // biting again, and holding it
        second = (++watchCount >= WATCH_CLENCH_SAMPLES);
      } else {
        watchCount = 0;
      }
    }
  }
  if (!second) return;

  if (seqPending == GEST_REVERSE && personalized && blinkPairUneven(seqP2PBefore, watchPeak - watchTrough)) {
    // FINAL: 1st blink clearly smaller = an everyday blink; this firm blink starts the command
    seqPendingMs = (uint32_t)millis();
    seqLevelBefore = levelBase;
    seqPeakBefore = watchPeak;
    seqP2PBefore = watchPeak - watchTrough;       // at least the size the watcher saw
    seqSwallow = GEST_REVERSE;                   // the engine's decision on this same blink
    seqSwallowUntilMs = (uint32_t)millis() + 300UL;
    watchPhase = 0;
    watchCount = 0;
    logDebug("[SEQ  %8.2fs] 1st blink was an everyday blink - this one starts the command\n", (double)millis() / 1000.0);
    return;
  }
  const Gesture g = seqPending;
  seqPending = GEST_NONE;
  seqSwallow = g;
  seqSwallowUntilMs = (uint32_t)millis() + ((g == GEST_REVERSE) ? SWALLOW_BLINK_MS : SWALLOW_CLENCH_MS);
  logDebug("[SEQ  %8.2fs] 2nd %s seen\n", (double)millis() / 1000.0, (g == GEST_REVERSE) ? "blink" : "clench");
  if (g == GEST_REVERSE) seqFire(GEST_LEFT);
  else talkDeferRight();                         // PRO: RIGHT after 0.9 s if no 3rd bite
}

static void setTurnPulse(uint32_t ms) {
  if (ms < TURN_PULSE_MIN_MS) ms = TURN_PULSE_MIN_MS;
  if (ms > TURN_PULSE_MAX_MS) ms = TURN_PULSE_MAX_MS;
  turnPulseMs = ms;
  bool saved = false;
  if (prefs.begin("neurox", false)) {            // kept across reboots, like personal training
    saved = prefs.putUInt("turnms", turnPulseMs) > 0;
    prefs.end();
  }
  txPrintf(true, "[CMD] turn time = %lu ms%s - press l / r to try it\n", (unsigned long)turnPulseMs,
           saved ? " (saved)" : " (not saved)");
}

static void loadTurnPulse() {
  uint32_t ms = TURN_PULSE_DEFAULT_MS;
  if (prefs.begin("neurox", false)) {
    ms = prefs.getUInt("turnms", TURN_PULSE_DEFAULT_MS);
    prefs.end();
  }
  if (ms < TURN_PULSE_MIN_MS || ms > TURN_PULSE_MAX_MS) ms = TURN_PULSE_DEFAULT_MS;
  turnPulseMs = ms;
}

// =============================================================================
// GESTURE ENGINE
// =============================================================================
static void preTriggerClear() {
  preHead = 0;
  preCount = 0;
}

static void preTriggerPush(float v, float emg, float level) {
  preV[preHead] = v;
  preEmg[preHead] = emg;
  preLevel[preHead] = level;
  preEmgV[preHead] = chV.emg;
  preEmgH[preHead] = chH.emg;
  preHead = (uint8_t)((preHead + 1) % PRE_TRIG_LEN);
  if (preCount < PRE_TRIG_LEN) ++preCount;
}

static void enterSettle() {
  gState = ST_SETTLE;
  settleSamples = 0;
  quietRun = 0;
  settleAfterBlink = false;
  settleJaw = 0;
}

static void latchBite(float base, float rise) {
  biteLatched = true;
  biteBase = base;
  biteRise = rise;
}

static void captureAdd(float v, float emg) {
  const float r = v - cap.base;
  if (r > cap.winVMax) {
    cap.winVMax = r;
    cap.retMin = 1.0e9f;                         // new peak: "after peak" restarts
  } else if (r < cap.retMin) {
    cap.retMin = r;
  }
  if (r < cap.winVMin) cap.winVMin = r;
  if (emg > cap.winVEmgMax) cap.winVEmgMax = emg;
  if (emg > thr.fwdEmg && cap.emgAboveCount < 255) ++cap.emgAboveCount;
  if (capTraceLen < CAP_TRACE_LEN) {
    capTraceV[capTraceLen] = chV.emg;
    capTraceH[capTraceLen] = chH.emg;
    ++capTraceLen;
  }
  ++cap.postCount;
}

// Seeds the window with the pre-trigger history (oldest -> newest), then the trigger sample.
static void captureStart(float v, float emg, TrigSource src, float rise) {
  cap.winVMax = -1.0e9f;
  cap.winVMin = 1.0e9f;
  cap.preVMin = 1.0e9f;
  cap.retMin = 1.0e9f;
  cap.winVEmgMax = 0.0f;
  cap.emgAboveCount = 0;
  cap.ruleBits = 0;
  cap.conflict = false;
  cap.fwdScore = 0.0f;
  cap.revScore = 0.0f;
  cap.onsetRise = rise;
  cap.source = src;
  cap.postCount = 0;
  cap.preCount = preCount;
  capTraceLen = 0;
  uint8_t idx = (uint8_t)((preHead + PRE_TRIG_LEN - preCount) % PRE_TRIG_LEN);
  cap.baseLevel = (src == TRIG_DC) ? levelBase : preLevel[idx];   // slow shift: pre-ring already elevated
  cap.base = (preCount >= 2) ? 0.5f * (preV[idx] + preV[(idx + 1) % PRE_TRIG_LEN])
                             : ((preCount == 1) ? preV[idx] : 0.0f);
  for (uint8_t k = 0; k < preCount; ++k) {
    const float s = preV[idx] - cap.base;
    if (s > cap.winVMax) {
      cap.winVMax = s;
      cap.retMin = 1.0e9f;
    } else if (s < cap.retMin) {
      cap.retMin = s;
    }
    if (s < cap.preVMin) cap.preVMin = s;
    if (preEmg[idx] > cap.winVEmgMax) cap.winVEmgMax = preEmg[idx];
    if (preEmg[idx] > thr.fwdEmg) ++cap.emgAboveCount;
    if (capTraceLen < CAP_TRACE_LEN) {
      capTraceV[capTraceLen] = preEmgV[idx];
      capTraceH[capTraceLen] = preEmgH[idx];
      ++capTraceLen;
    }
    idx = (uint8_t)((idx + 1) % PRE_TRIG_LEN);
  }
  captureAdd(v, emg);
}

static void finishCapture(Gesture g, const char *why) {
  const float fullMin = (cap.preVMin < cap.winVMin) ? cap.preVMin : cap.winVMin;
  const float p2p = cap.winVMax - fullMin;
  const float winMs = (float)(cap.preCount + cap.postCount) * 1000.0f / FS_F;

  const char *seqAction = "no pulse";
  if (g == GEST_NONE) {
    ++ignoredCount;
  } else {
    gestP2P = p2p;                               // PRO: blink size for the soft-blink check
    seqAction = seqGesture(g);                   // steering sequencer decides what is sent
  }

  char retText[16];
  if (cap.retMin < 1.0e8f) snprintf(retText, sizeof(retText), "%+.0f", (double)cap.retMin);
  else snprintf(retText, sizeof(retText), "n/a");
  char scoreText[32];
  if (cap.conflict) {
    snprintf(scoreText, sizeof(scoreText), " score F%.2f/R%.2f", (double)cap.fwdScore, (double)cap.revScore);
  } else {
    scoreText[0] = '\0';
  }
  const char *trigText = (cap.source == TRIG_POS)   ? "POS"
                         : (cap.source == TRIG_NEG) ? "NEG"
                         : (cap.source == TRIG_EMG) ? "EMG"
                                                    : "DC";
  const char *gestText = (g == GEST_FORWARD) ? "FORWARD" : (g == GEST_REVERSE) ? "REVERSE" : "ignored";
  const char *action = seqAction;

  // rules=EeSs/DdBb : which FORWARD (EMG, Shape) and REVERSE (Deep, Biphasic) rules matched
  logDebug("[GEST %8.2fs] %-7s %-24s | winVMax=%+.0f winVMin=%+.0f P2P=%.0f ret=%s EMG=%.0f n=%u | rules=E%uS%u/D%uB%u%s | base=%+.0f edge=%.0f %s %.0fms -> %s\n",
           (double)millis() / 1000.0, gestText, why, (double)cap.winVMax, (double)cap.winVMin, (double)p2p,
           retText, (double)cap.winVEmgMax, (unsigned)cap.emgAboveCount, (unsigned)(cap.ruleBits & 1u),
           (unsigned)((cap.ruleBits >> 1) & 1u), (unsigned)((cap.ruleBits >> 2) & 1u),
           (unsigned)((cap.ruleBits >> 3) & 1u), scoreText, (double)cap.base, (double)cap.onsetRise, trigText,
           (double)winMs, action);

  // Bite-hold latch: any non-blink event that shifted the DC level (a FORWARD, or a
  // bite too weak to be one) keeps the engine in SETTLE until the bite is released.
  // Through the 0.5 Hz high-pass, both the held shift (undershoot) and its release
  // produce negative swings that would otherwise read as a blink.
  biteLatched = false;
  if (g != GEST_REVERSE) {
    const float rise = chV.level - cap.baseLevel;
    if (rise >= thr.biteLatchMin) latchBite(cap.baseLevel, rise);
  }
  enterSettle();
  settleAfterBlink = (g == GEST_REVERSE);         // FINAL: see settleStep()
}

// Decides only when the full post-onset window (CAPTURE_POST_LEN samples) has been
// seen: no mid-window exits, so a clench whose tail dips negative is judged on its
// complete morphology.
static void captureEvaluate() {
  if (cap.postCount < CAPTURE_POST_LEN) return;

  const float fullMin = (cap.preVMin < cap.winVMin) ? cap.preVMin : cap.winVMin;
  const float p2p = cap.winVMax - fullMin;
  const float dip = (cap.winVMin < 0.0f) ? -cap.winVMin : 0.0f;

  if (trainPhase == TRAIN_BLINK || trainPhase == TRAIN_CLENCH) {   // personal training: record, never drive
    if (acPhase != AC_OFF) acCapture(p2p);       // FINAL: auto-calibration keeps the best answer to a beep
    else trainRecord(p2p);
    return;
  }

  // A capture opened by a slow DC shift starts mid-event, so its V shape is not
  // meaningful: it can only become a FORWARD through the EMG rule, never a REVERSE.
  const bool shapeValid = (cap.source != TRIG_DC);
  // FORWARD (jaw clench)
  const bool fwdEmg = cap.emgAboveCount >= emgMinCount;
  const bool fwdShape = shapeValid && (cap.winVMax >= thr.fwdVMax) && (cap.winVMin >= thr.fwdMinFloor) &&
                        (cap.winVMax > FWD_DOMINANCE * dip);
  // REVERSE (eyelid blink)
  const bool revDeep = shapeValid && (cap.winVMin <= thr.revDeepMin) && (cap.winVMax >= thr.deepPeakMin);
  const bool revBi = shapeValid && (cap.winVMax >= thr.biVMax) && (p2p >= thr.biP2P) &&
                     (cap.retMin <= thr.biRetMin || retFree);   // FINAL: slow blinks return after the window

  cap.ruleBits = (uint8_t)((fwdEmg ? 1u : 0u) | (fwdShape ? 2u : 0u) | (revDeep ? 4u : 0u) | (revBi ? 8u : 0u));
  const bool fwd = fwdEmg || fwdShape;
  const bool rev = revDeep || revBi;

  if (fwd && rev) {
    cap.conflict = true;
    // Normalised dominance: each side's strongest evidence over its own threshold.
    const float emgRatio = cap.winVEmgMax / thr.fwdEmg;
    const float peakRatio = cap.winVMax / thr.fwdVMax;
    cap.fwdScore = (emgRatio > peakRatio) ? emgRatio : peakRatio;
    const float deepRatio = cap.winVMin / thr.revDeepMin;                 // both negative -> positive ratio
    const float retRatio = (cap.retMin < 1.0e8f) ? (cap.retMin / thr.biRetMin) : 0.0f;
    cap.revScore = (deepRatio > retRatio) ? deepRatio : retRatio;
    if (cap.emgAboveCount >= emgStrongCount) {
      finishCapture(GEST_FORWARD, "both: sustained EMG wins");
    } else if (cap.fwdScore >= cap.revScore) {
      finishCapture(GEST_FORWARD, "both: bite dominant");
    } else {
      finishCapture(GEST_REVERSE, "both: blink dominant");
    }
  } else if (fwd) {
    finishCapture(GEST_FORWARD, fwdEmg ? "bite: sustained EMG" : "bite: positive shift");
  } else if (rev) {
    finishCapture(GEST_REVERSE, revDeep ? "blink: deep trough" : "blink: biphasic swing");
  } else {
    finishCapture(GEST_NONE, "below FWD/REV rules");
  }
}

static void settleStep(float v, float emg, float level) {
  ++settleSamples;
  bool quiet = (fabsf(v) < thr.quietV) && (emg < thr.quietEmg);

  if (biteLatched) {
    const float shift = level - biteBase;
    if (shift > biteRise) biteRise = shift;      // track the peak of the held shift
    if (shift < BITE_RELEASE_FRAC * biteRise) {
      biteLatched = false;
      if (holdoffSamples < BITE_RELEASE_HOLDOFF) holdoffSamples = BITE_RELEASE_HOLDOFF;
      quiet = false;
      logDebug("[BITE] released after %.0f ms (peak shift %+.0f) - absorbing release rebound\n",
               (double)settleSamples * 1000.0 / (double)FS_HZ, (double)biteRise);
    } else if (settleSamples >= BITE_LATCH_MAX) {
      biteLatched = false;
      levelBase = level;                         // accept the new DC level as baseline
      txPrintf(true, "[WARN] bite hold > %lu s (or DC drift): re-baselined, re-arming on quiet signal\n",
               (unsigned long)(BITE_LATCH_MAX / FS_HZ));
    } else {
      quiet = false;
    }
  }

  if (holdoffSamples > 0) {
    --holdoffSamples;
    quiet = false;
  }
  // FINAL: a blink just before a clench (an everyday blink, or a REVERSE then FORWARD):
  // the clench keeps the signal busy, so waiting for quiet would swallow it. After the
  // blink's refractory time, sustained jaw EMG re-arms the engine at once.
  if (settleAfterBlink && !biteLatched && holdoffSamples == 0 && settleSamples >= REFRACTORY_SAMPLES) {
    settleJaw = (emg > thr.fwdEmg) ? (uint16_t)(settleJaw + 1) : (uint16_t)0;
    if (settleJaw >= SETTLE_JAW_SAMPLES) {
      gState = ST_ARMED;
      slowEdgeNoted = false;
      settleAfterBlink = false;
      logDebug("[ARMED] jaw clench right after a blink - catching it\n");
      return;
    }
  }

  quietRun = quiet ? (uint8_t)((quietRun < 255) ? quietRun + 1 : 255) : (uint8_t)0;

  if (settleSamples >= REFRACTORY_SAMPLES && quietRun >= QUIET_RUN_REQUIRED) {
    gState = ST_ARMED;
    slowEdgeNoted = false;
    logDebug("[ARMED] after %.0f ms settle\n", (double)settleSamples * 1000.0 / (double)FS_HZ);
    return;
  }
  if ((settleSamples % SETTLE_WARN_SAMPLES) == 0) {
    txPrintf(true, "[WARN] still settling after %lu s: |V|=%.0f (need <%.0f) EMG=%.0f (need <%.0f)%s - relax, or press s / c\n",
             (unsigned long)(settleSamples / FS_HZ), (double)fabsf(v), (double)thr.quietV, (double)emg,
             (double)thr.quietEmg, biteLatched ? " [bite held]" : "");
  }
}

static void gestureStep(float v, float emg, float level) {
  if (satV || vOnRail) {                         // clipped sample / electrode off: no decisions
    if (gState == ST_CAPTURE) logDebug("[GEST] capture aborted: vertical channel clipped\n");
    if (gState != ST_SETTLE) enterSettle();
    return;
  }

  if (!biteLatched) levelBase += LEVEL_BASE_ALPHA * (level - levelBase);

  switch (gState) {
    case ST_ARMED: {
      if (preCount < PRE_TRIG_LEN) return;       // need a full onset history
      const bool pos = (v >= thr.trigV) || dipRiseOnset(v);   // FINAL: also a blink rising out of a dip
      const bool neg = v <= -thr.trigV;
      const bool emgAssist = (emg > thr.fwdEmg) && (v >= thr.emgTrigVMin);
      const float dcShift = level - levelBase;
      bool open = false;
      TrigSource src = TRIG_DC;
      float rise = dcShift;

      if (pos || neg || emgAssist) {
        float pMin = preV[0], pMax = preV[0];
        for (uint8_t i = 1; i < PRE_TRIG_LEN; ++i) {
          if (preV[i] < pMin) pMin = preV[i];
          if (preV[i] > pMax) pMax = preV[i];
        }
        if (neg) {
          src = TRIG_NEG;
          rise = pMax - v;
        } else {
          src = pos ? TRIG_POS : TRIG_EMG;
          rise = v - pMin;
        }
        open = true;
        if (src != TRIG_EMG && rise < thr.onsetRise) {
          if (src == TRIG_POS && slowBlinkOnset(v)) {
            rise = v - slowRingMin();            // FINAL: slow, but a big clean rise in 125 ms = a blink
          } else if (emgAssist) {
            src = TRIG_EMG;                      // slow edge, but muscle-backed
          } else {
            open = false;
            if (!slowEdgeNoted) {
              slowEdgeNoted = true;
              ++slowEdgeCount;
              logDebug("[SKIP] slow excursion V=%+.0f (edge %.0f < %.0f in 39 ms): filter tail/drift, not a gesture\n",
                       (double)v, (double)rise, (double)thr.onsetRise);
            }
          }
        }
      } else {
        slowEdgeNoted = false;
      }

      // Slow positive DC shift with no sharp onset (slow clench, chewing): open a
      // capture anyway so the EMG rule can still call it a bite; if it is not
      // one, the window end latches it so its release is absorbed.
      if (!open && dcShift >= thr.biteLevel) {
        open = true;
        src = TRIG_DC;
        rise = dcShift;
      }
      if (!open) return;

      captureStart(v, emg, src, rise);
      gState = ST_CAPTURE;
      captureEvaluate();
      return;
    }
    case ST_CAPTURE:
      captureAdd(v, emg);
      captureEvaluate();
      return;
    case ST_SETTLE:
      settleStep(v, emg, level);
      return;
  }
}

// Clipping / electrode-off guard (raw ADC on a rail). Any clipped sample aborts a
// capture and delays re-arming by 1 s. A sustained clip (electrode off) pauses
// detection; when it clears, the vertical filters are reset (their 0.5 Hz
// high-pass would otherwise ring for seconds) and a 2 s hold-off is applied.
static void updateSaturation(float rawH, float rawV) {
  vOnRail = (rawV <= ADC_RAIL_LOW) || (rawV >= ADC_RAIL_HIGH);
  if (vOnRail) {
    clearRunV = 0;
    if (satRunV < 0xFFFF) ++satRunV;
    if (holdoffSamples < CLIP_HOLDOFF_SAMPLES) holdoffSamples = CLIP_HOLDOFF_SAMPLES;
    if (!satV && satRunV >= SAT_TRIP_SAMPLES) {
      satV = true;
      txPrintf(true, "[WARN] VERTICAL channel clipped (raw %.0f): electrode off or loose - detection paused\n",
               (double)rawV);
    }
  } else {
    satRunV = 0;
    if (clearRunV < 0xFFFF) ++clearRunV;
    if (satV && clearRunV >= SAT_CLEAR_SAMPLES) {
      satV = false;
      chV.reset();
      preTriggerClear();
      biteLatched = false;
      levelBase = 0.0f;
      holdoffSamples = WARMUP_SAMPLES;
      enterSettle();
      txPrintf(true, "[INFO] VERTICAL channel back in range - filters reset, re-arming in ~2 s (press c if electrodes were re-seated)\n");
    }
  }

  const bool railH = (rawH <= ADC_RAIL_LOW) || (rawH >= ADC_RAIL_HIGH);
  if (railH) {
    clearRunH = 0;
    if (satRunH < 0xFFFF) ++satRunH;
    if (!satH && satRunH >= SAT_TRIP_SAMPLES) {
      satH = true;
      txPrintf(true, "[WARN] HORIZONTAL channel clipped (raw %.0f): telemetry only, control unaffected\n",
               (double)rawH);
    }
  } else {
    satRunH = 0;
    if (clearRunH < 0xFFFF) ++clearRunH;
    if (satH && clearRunH >= SAT_CLEAR_SAMPLES) {
      satH = false;
      txPrintf(true, "[INFO] HORIZONTAL channel back in range\n");
    }
  }
}

// =============================================================================
// PERSONAL TRAINING
// The normal capture engine keeps running (made x0.70 more sensitive so every
// gesture opens a capture); each finished capture is recorded instead of
// driving the car. From 5 blinks and 5 clenches the base thresholds are set:
//   jaw EMG  : geometric middle between the strongest blink EMG and the
//              weakest clench EMG (2nd lowest of 5, so one weak clench is ok)
//   blink    : 60 % of your median blink peak / swing, 50 % of its return
//   trigger  : 60 % of your median blink peak (quiet / edge keep v13.2 ratios)
//   shape-FWD: raised above your biggest blink so a blink can never match it
// =============================================================================
static float median5(const float *v) {
  float s[TRAIN_SAMPLES];
  for (uint8_t i = 0; i < TRAIN_SAMPLES; ++i) s[i] = v[i];
  for (uint8_t i = 1; i < TRAIN_SAMPLES; ++i) {
    const float key = s[i];
    int8_t j = (int8_t)(i - 1);
    while (j >= 0 && s[j] > key) { s[j + 1] = s[j]; --j; }
    s[j + 1] = key;
  }
  return s[TRAIN_SAMPLES / 2];
}

static float secondLowest5(const float *v) {
  float lo1 = 1.0e9f, lo2 = 1.0e9f;
  for (uint8_t i = 0; i < TRAIN_SAMPLES; ++i) {
    if (v[i] < lo1) { lo2 = lo1; lo1 = v[i]; }
    else if (v[i] < lo2) { lo2 = v[i]; }
  }
  return lo2;
}

static void trainPrompt() {
  if (trainPhase == TRAIN_BLINK) {
    txPrintf(true, "[TRAIN] Step 1/3: BLINK firmly 5 times, jaw relaxed, eyes looking straight ahead.\n");
    txPrintf(true, "        Blink once, wait for the 'blink n/5' line, then blink again.\n");
  } else {
    txPrintf(true, "[TRAIN] Step 2/3: CLENCH your jaw 5 times: bite down ~1 s, relax, eyes still.\n");
    txPrintf(true, "        Clench once, wait for the 'clench n/5' line, then clench again.\n");
  }
}

static void trainStart() {
  if (sysMode != MODE_RUN || satV) {
    txPrintf(true, "[TRAIN] not now: wait until the [READY] line (and electrodes connected), then press t\n");
    return;
  }
  if (trainPhase != TRAIN_OFF) trainAbort("restarted");
  motorStopAll();
  if (lockState != LOCK_LOCKED) lockEngage("training");
  trainSavedBase = base;
  trainSavedThrScale = thrScale;
  trainSavedBiteScale = biteScale;
  trainSavedMinCount = emgMinCount;
  trainSavedStrongCount = emgStrongCount;
  trainSavedSource = emgSource;
  emgSource = EMG_SRC_BOTH;                      // capture with v13.2 EMG ...
  base = FACTORY_BASE;                           // ... and factory thresholds ...
  thrScale = TRAIN_CAPTURE_SCALE;                // ... made more sensitive
  biteScale = 1.0f;
  applyScales();
  trainPhase = TRAIN_BLINK;
  trainCount = 0;
  trainStepStartMs = (uint32_t)millis();
  preTriggerClear();
  biteLatched = false;
  holdoffSamples = 0;
  enterSettle();
  txPrintf(true, "\n[TRAIN] ===== PERSONAL TRAINING (the car will not move) =====\n");
  trainPrompt();
}

static void trainAbort(const char *why) {
  if (acPhase != AC_OFF) {                       // FINAL: auto-calibration interrupted
    acPhase = AC_OFF;
    acWinOpen = false;
    trainPhase = TRAIN_OFF;
    base = trainSavedBase;
    thrScale = trainSavedThrScale;
    biteScale = trainSavedBiteScale;
    emgMinCount = trainSavedMinCount;
    emgStrongCount = trainSavedStrongCount;
    emgSource = trainSavedSource;
    applyScales();
    txPrintf(true, "[AUTO] calibration cancelled (%s): previous settings kept\n", why);
    return;
  }
  if (trainPhase == TRAIN_SQUEEZE) {             // steps 1-2 are already saved: keep them
    trainPhase = TRAIN_OFF;
    txPrintf(true, "[TRAIN] unlock step cancelled (%s): blink/clench training kept, unlock level %.0f kept\n", why,
             (double)unlockLevel);
    lockEngage("training cancelled");
    return;
  }
  base = trainSavedBase;
  thrScale = trainSavedThrScale;
  biteScale = trainSavedBiteScale;
  emgMinCount = trainSavedMinCount;
  emgStrongCount = trainSavedStrongCount;
  emgSource = trainSavedSource;
  applyScales();
  trainPhase = TRAIN_OFF;
  trainCount = 0;
  txPrintf(true, "[TRAIN] cancelled (%s): previous thresholds kept\n", why);
}

static void trainRecord(float p2p) {
  TrainSample smp;
  smp.vmax = cap.winVMax;
  smp.vmin = cap.winVMin;
  smp.p2p = p2p;
  smp.ret = (cap.retMin < 1.0e8f) ? cap.retMin : 0.0f;
  smp.emg = cap.winVEmgMax;
  smp.emgCount = cap.emgAboveCount;
  smp.traceLen = capTraceLen;
  for (uint8_t i = 0; i < capTraceLen; ++i) {
    smp.traceV[i] = capTraceV[i];
    smp.traceH[i] = capTraceH[i];
  }
  const bool blinkStep = (trainPhase == TRAIN_BLINK);
  (blinkStep ? trainBlink : trainClench)[trainCount++] = smp;
  buzz(BZ_REC, sizeof(BZ_REC) / sizeof(BZ_REC[0]));
  txPrintf(true, "[TRAIN] %s %u/%u: peak %+.0f dip %+.0f swing %.0f return %+.0f EMG %.0f\n",
           blinkStep ? "blink" : "clench", (unsigned)trainCount, (unsigned)TRAIN_SAMPLES, (double)smp.vmax,
           (double)smp.vmin, (double)smp.p2p, (double)smp.ret, (double)smp.emg);

  // Same end-of-capture handling as finishCapture(): a clench latches the bite
  // so its release is absorbed; then SETTLE until the signal is quiet again.
  biteLatched = false;
  if (!blinkStep) {
    const float rise = chV.level - cap.baseLevel;
    if (rise >= thr.biteLatchMin) latchBite(cap.baseLevel, rise);
  }
  enterSettle();

  if (trainCount < TRAIN_SAMPLES) return;
  if (blinkStep) {
    trainPhase = TRAIN_CLENCH;
    trainCount = 0;
    trainStepStartMs = (uint32_t)millis();
    trainPrompt();
  } else {
    trainFinish();
  }
}

static void trainFinish() {
  float bV[TRAIN_SAMPLES], bMin[TRAIN_SAMPLES], bP2P[TRAIN_SAMPLES], bRet[TRAIN_SAMPLES];
  float cEmg[TRAIN_SAMPLES];
  float blinkEmgHi = 0.0f, blinkVHi = 0.0f, clenchVMinLo = 1.0e9f;
  for (uint8_t i = 0; i < TRAIN_SAMPLES; ++i) {
    bV[i] = trainBlink[i].vmax;
    bMin[i] = trainBlink[i].vmin;
    bP2P[i] = trainBlink[i].p2p;
    bRet[i] = trainBlink[i].ret;
    cEmg[i] = trainClench[i].emg;
    if (trainBlink[i].emg > blinkEmgHi) blinkEmgHi = trainBlink[i].emg;
    if (trainBlink[i].vmax > blinkVHi) blinkVHi = trainBlink[i].vmax;
    if (trainClench[i].vmin < clenchVMinLo) clenchVMinLo = trainClench[i].vmin;
  }
  const float mV = median5(bV), mMin = median5(bMin), mP2P = median5(bP2P), mRet = median5(bRet);
  const float clenchEmgLo = secondLowest5(cEmg);
  const float clenchEmgMed = median5(cEmg);

  // ---- jaw EMG threshold: level AND duration ----
  // For candidate levels, count the window samples above the level in every
  // recorded capture. Pick the level where the weakest clenches (2nd lowest)
  // stay above it for the most samples MORE than the strongest blink does,
  // and put the sample-count rule in the middle of that gap.
  float fwd = TRAIN_EMG_FLOOR;
  int bestGap = -1000, bestBlinkN = 0, bestClenchN = 0;
  EmgSource bestSrc = EMG_SRC_BOTH;
  for (uint8_t src = EMG_SRC_BOTH; src <= EMG_SRC_FOREHEAD; ++src) {   // v13.2 source first: wins ties
    for (uint8_t step = 0; step <= 40; ++step) {
      const float level = TRAIN_EMG_FLOOR * powf(1.06f, (float)step);   // 35 .. ~360, 6 % steps
      int blinkN = 0;
      float clenchN[TRAIN_SAMPLES];
      for (uint8_t i = 0; i < TRAIN_SAMPLES; ++i) {
        int nb = 0, nc = 0;
        const TrainSample &b = trainBlink[i], &c = trainClench[i];
        for (uint8_t k = 0; k < b.traceLen; ++k) {
          const float e = (src == EMG_SRC_TEMPLES) ? b.traceH[k] : (src == EMG_SRC_FOREHEAD) ? b.traceV[k]
                                                                  : fmaxf(b.traceV[k], b.traceH[k]);
          if (e * TRAIN_BLINK_STRESS > level) ++nb;
        }
        for (uint8_t k = 0; k < c.traceLen; ++k) {
          const float e = (src == EMG_SRC_TEMPLES) ? c.traceH[k] : (src == EMG_SRC_FOREHEAD) ? c.traceV[k]
                                                                  : fmaxf(c.traceV[k], c.traceH[k]);
          if (e * TRAIN_CLENCH_STRESS > level) ++nc;
        }
        if (nb > blinkN) blinkN = nb;
        clenchN[i] = (float)nc;
      }
      const int clenchLo = (int)secondLowest5(clenchN);
      const int gap = clenchLo - blinkN;
      // A different source must be clearly better (+3 samples) to replace v13.2's;
      // within a source, ties go to the higher (safer) level.
      const bool better = (src == bestSrc) ? (gap >= bestGap) : (gap >= bestGap + 3);
      if (better) {
        bestGap = gap;
        bestBlinkN = blinkN;
        bestClenchN = clenchLo;
        bestSrc = (EmgSource)src;
        fwd = level;
      }
    }
  }
  const bool separated = bestGap >= 1;
  const bool goodSeparation = bestGap >= (int)TRAIN_MIN_GAP;
  uint8_t minCount, strongCount;
  EmgSource newSource = bestSrc;
  if (separated) {
    minCount = (uint8_t)(bestBlinkN + (bestGap + 1) / 2);   // middle of the (stress-tested) gap
    if (minCount < EMG_MIN_COUNT) minCount = EMG_MIN_COUNT;
    // A capture that ALSO looks like a blink is only called FORWARD if its EMG
    // lasts like a real clench: 80 % of your weakest clench (2nd lowest of 5).
    float rawN[TRAIN_SAMPLES];
    for (uint8_t i = 0; i < TRAIN_SAMPLES; ++i) {
      const TrainSample &c = trainClench[i];
      int n = 0;
      for (uint8_t k = 0; k < c.traceLen; ++k) {
        const float e = (bestSrc == EMG_SRC_TEMPLES) ? c.traceH[k] : (bestSrc == EMG_SRC_FOREHEAD) ? c.traceV[k]
                                                                    : fmaxf(c.traceV[k], c.traceH[k]);
        if (e > fwd) ++n;
      }
      rawN[i] = (float)n;
    }
    const int strongN = (int)(TRAIN_STRONG_FRAC * secondLowest5(rawN));
    strongCount = (strongN > (int)minCount) ? (uint8_t)strongN : minCount;
  } else {
    // No clean separation: protect blinks (level above every blink burst),
    // keep the factory sample counts, and tell the user.
    fwd = TRAIN_BLINK_EMG_MARGIN * blinkEmgHi;
    if (fwd < TRAIN_EMG_FLOOR) fwd = TRAIN_EMG_FLOOR;
    minCount = EMG_MIN_COUNT;
    strongCount = EMG_STRONG_COUNT;
    newSource = EMG_SRC_BOTH;
  }

  // ---- blink + trigger thresholds ----
  BaseThresholds nb = FACTORY_BASE;
  float trig = TRAIN_BLINK_FRAC * mV;
  if (trig < 40.0f) trig = 40.0f;
  nb.trigV = trig;
  nb.quietV = trig * (BASE_QUIET_V / BASE_TRIG_V);
  nb.onsetRise = trig * (BASE_ONSET_RISE / BASE_TRIG_V);
  nb.emgTrigVMin = trig * (BASE_EMG_TRIG_VMIN / BASE_TRIG_V);
  nb.biVMax = TRAIN_BLINK_FRAC * mV;
  nb.biP2P = TRAIN_BLINK_FRAC * mP2P;
  nb.biRetMin = TRAIN_RET_FRAC * mRet;
  // FINAL: slow blinks (tired, elderly, myasthenia) come back down only after the
  // capture window: then the return is not required (looking up is still caught
  // by the look-up check, which sees the eye level stay shifted)
  retFree = (mRet > SLOW_RET_MAX);
  if (retFree) txPrintf(true, "[TRAIN] your blinks are slow (no return within 0.2 s) - the return is not required\n");
  if (nb.biRetMin > -15.0f) nb.biRetMin = -15.0f;
  float deep = TRAIN_BLINK_FRAC * mMin;          // negative
  if (deep > -0.6f * trig) deep = -0.6f * trig;
  if (clenchVMinLo < 0.0f && deep > 1.25f * clenchVMinLo) deep = 1.25f * clenchVMinLo;  // deeper than any clench dip
  nb.revDeepMin = deep;
  // ---- jaw ----
  nb.fwdEmg = fwd;
  nb.quietEmg = fwd * (BASE_QUIET_EMG / BASE_FWD_EMG);
  nb.fwdVMax = (1.4f * blinkVHi > BASE_FWD_VMAX) ? 1.4f * blinkVHi : BASE_FWD_VMAX;
  if (newSource == EMG_SRC_TEMPLES) nb.emgTrigVMin = -1000.0f;   // temple EMG is jaw-specific: no V rise needed

  // Backup rules that eye movements can trigger are switched off when your own
  // samples show they are not needed:
  //  - jaw "big positive shift" rule: a large UPWARD look matches it (false FORWARD).
  //    Off when the trained EMG rule alone recognises your clenches.
  //  - blink "deep dip" rule: a DOWNWARD look matches it (false REVERSE). A
  //    blink has an upward phase, a downward look has none, so when ALL 5 of
  //    your blinks show an upward phase, the dip rule requires one too.
  float blinkVLo = 1.0e9f;
  for (uint8_t i = 0; i < TRAIN_SAMPLES; ++i) if (trainBlink[i].vmax < blinkVLo) blinkVLo = trainBlink[i].vmax;
  // The shape rule is only a backup for clenches EMG cannot see: switch it off
  // when the trained EMG rule alone recognises at least 4 of your 5 clenches.
  uint8_t clenchByEmg = 0;
  for (uint8_t i = 0; i < TRAIN_SAMPLES; ++i) {
    const TrainSample &c = trainClench[i];
    uint8_t n = 0;
    for (uint8_t k = 0; k < c.traceLen; ++k) {
      const float e = (newSource == EMG_SRC_TEMPLES) ? c.traceH[k] : (newSource == EMG_SRC_FOREHEAD) ? c.traceV[k]
                                                                       : fmaxf(c.traceV[k], c.traceH[k]);
      if (e > fwd) ++n;
    }
    if (n >= minCount) ++clenchByEmg;
  }
  const bool shapeJawOff = separated && clenchByEmg >= TRAIN_SAMPLES - 1;
  const float peakNeed = TRAIN_DEEP_PEAK_FRAC * mV;
  const bool deepNeedsPeak = (mV > 0.0f) && (blinkVLo >= 1.3f * peakNeed);
  if (shapeJawOff) nb.fwdVMax = RULE_OFF;
  if (deepNeedsPeak) nb.deepPeakMin = peakNeed;

  // ---- self-check on the recorded samples ----
  uint8_t blinkOk = 0, clenchOk = 0;
  for (uint8_t i = 0; i < TRAIN_SAMPLES; ++i) {
    const TrainSample &b = trainBlink[i];
    const bool rev = (b.vmin <= nb.revDeepMin) || (b.vmax >= nb.biVMax && b.p2p >= nb.biP2P && (b.ret <= nb.biRetMin || retFree));
    const TrainSample &c = trainClench[i];
    uint8_t nB = 0, nC = 0;
    for (uint8_t k = 0; k < b.traceLen; ++k) {
      const float e = (newSource == EMG_SRC_TEMPLES) ? b.traceH[k] : (newSource == EMG_SRC_FOREHEAD) ? b.traceV[k]
                                                                       : fmaxf(b.traceV[k], b.traceH[k]);
      if (e > nb.fwdEmg) ++nB;
    }
    for (uint8_t k = 0; k < c.traceLen; ++k) {
      const float e = (newSource == EMG_SRC_TEMPLES) ? c.traceH[k] : (newSource == EMG_SRC_FOREHEAD) ? c.traceV[k]
                                                                       : fmaxf(c.traceV[k], c.traceH[k]);
      if (e > nb.fwdEmg) ++nC;
    }
    if (rev && nB < strongCount) ++blinkOk;
    if (nC >= minCount) ++clenchOk;
  }

  acBlinkOk = blinkOk;                           // FINAL: auto-calibration judges these
  acClenchOk = clenchOk;
  acSeparated = separated;
  base = nb;
  emgMinCount = minCount;
  emgStrongCount = strongCount;
  emgSource = newSource;
  personalized = true;
  thrScale = SCALE_DEFAULT;
  biteScale = 1.0f;
  applyScales();
  trainPhase = TRAIN_OFF;
  if (acPhase == AC_OFF) trainSave();            // FINAL: auto-calibration saves only a good result

  txPrintf(true, "\n[TRAIN] ===== RESULT (saved - loaded automatically at every boot) =====\n");
  txPrintf(true, "[TRAIN] your blink : peak %+.0f  dip %+.0f  swing %.0f  return %+.0f  | blink EMG up to %.0f\n",
           (double)mV, (double)mMin, (double)mP2P, (double)mRet, (double)blinkEmgHi);
  txPrintf(true, "[TRAIN] your clench: EMG typical %.0f, weakest %.0f\n", (double)clenchEmgMed, (double)clenchEmgLo);
  static const char *const srcName[] = {"both pills (as v13.2)", "TEMPLE pill only", "FOREHEAD pill only"};
  txPrintf(true, "[TRAIN] jaw EMG source: %s | above EMG %.0f: blinks up to %d samples, clenches at least %d (of %u)\n",
           srcName[newSource], (double)nb.fwdEmg, separated ? bestBlinkN : 0, separated ? bestClenchN : 0,
           (unsigned)CAP_TRACE_LEN);
  txPrintf(true, "[TRAIN] JAW = EMG>%.0f on >=%u samples (>=%u if it also looks like a blink) (factory 70 / 4 / 6)\n",
           (double)nb.fwdEmg, (unsigned)minCount, (unsigned)strongCount);
  if (deepNeedsPeak) {
    txPrintf(true, "[TRAIN] BLINK = peak>=%.0f swing>=%.0f return<=%+.0f, or dip<=%+.0f with peak>=%.0f (looking down cannot reverse)\n",
             (double)nb.biVMax, (double)nb.biP2P, (double)nb.biRetMin, (double)nb.revDeepMin, (double)nb.deepPeakMin);
  } else {
    txPrintf(true, "[TRAIN] BLINK = peak>=%.0f swing>=%.0f return<=%+.0f, or dip<=%+.0f\n", (double)nb.biVMax,
             (double)nb.biP2P, (double)nb.biRetMin, (double)nb.revDeepMin);
  }
  if (shapeJawOff) {
    txPrintf(true, "[TRAIN] jaw 'big positive shift' rule OFF: looking up cannot drive forward\n");
  }
  txPrintf(true, "[TRAIN] check on your samples: blinks -> REVERSE %u/%u, clenches -> FORWARD %u/%u\n",
           (unsigned)blinkOk, (unsigned)TRAIN_SAMPLES, (unsigned)clenchOk, (unsigned)TRAIN_SAMPLES);
  if (!separated) {
    txPrintf(true, "[TRAIN] WARNING: your clenches (EMG %.0f) are not clearly longer/stronger than your blinks (EMG %.0f).\n",
             (double)clenchEmgLo, (double)blinkEmgHi);
    txPrintf(true, "        Blinks are protected, but clenches may not register. Clench harder, or move the\n");
    txPrintf(true, "        temple pads onto the temple muscle (feel it bulge when you bite), then press t again.\n");
  } else if (!goodSeparation || blinkOk < TRAIN_SAMPLES - 1 || clenchOk < TRAIN_SAMPLES - 1) {
    txPrintf(true, "[TRAIN] Some samples were inconsistent - press NEW (or t) to calibrate again for a better result.\n");
  } else {
    txPrintf(true, "[TRAIN] Good separation. Blink = REVERSE, clench = FORWARD.\n");
  }
  printThresholds("Personal training");
  squeezeStart();                                // step 3: measure the unlock squeeze
}

static void trainService() {
  if (acPhase != AC_OFF) return;                 // FINAL: auto-calibration has its own timing
  if (trainPhase != TRAIN_BLINK && trainPhase != TRAIN_CLENCH) return;
  if ((uint32_t)((uint32_t)millis() - trainStepStartMs) >= TRAIN_TIMEOUT_MS) {
    trainAbort("timed out - got fewer than 5 gestures");
  }
}

static bool trainLoad() {
  SavedTraining st;
  bool ok = false;
  if (prefs.begin("neurox", false)) {
    if (prefs.getBytesLength("train") == sizeof(st) && prefs.getBytes("train", &st, sizeof(st)) == sizeof(st) &&
        st.magic == TRAIN_MAGIC) {
      base = st.base;
      emgMinCount = st.emgMinCount;
      emgStrongCount = st.emgStrongCount;
      emgSource = (st.emgSource <= EMG_SRC_FOREHEAD) ? (EmgSource)st.emgSource : EMG_SRC_BOTH;
      if (st.unlockLevel >= UNLOCK_MIN_LEVEL && st.unlockLevel < 10000.0f) unlockLevel = st.unlockLevel;
      unlockRatio = (st.unlockRatio > 0.0f && st.unlockRatio < 100.0f) ? st.unlockRatio : 0.0f;
      unlockHMax = (st.unlockHMax > 0.0f && st.unlockHMax < 10000.0f) ? st.unlockHMax : 0.0f;
      closeEogNeed = (st.closeEogNeed > 0.0f && st.closeEogNeed < 5000.0f) ? st.closeEogNeed : 0.0f;
      closeEogOff = st.closeEogOff != 0;
      unlockBite = st.unlockBite != 0;
      biteUnlockLevel = (st.biteLevel > 0.0f && st.biteLevel < 10000.0f) ? st.biteLevel : 0.0f;
      biteAvgNeed = (st.biteAvg > 0.0f && st.biteAvg < 10000.0f) ? st.biteAvg : 0.0f;
      retFree = st.retFree == 1;
      clenchDurMs = (st.clenchDur >= 200 && st.clenchDur <= 3000) ? st.clenchDur : 700;
      unlockAvgNeed = (st.holdAvg > 0.0f && st.holdAvg < 10000.0f) ? st.holdAvg : 0.0f;
      applyClenchTiming();
      personalized = true;
      ok = true;
    }
    prefs.end();
  }
  return ok;
}

static void trainSave() {
  SavedTraining st;
  st.magic = TRAIN_MAGIC;
  st.base = base;
  st.emgMinCount = emgMinCount;
  st.emgStrongCount = emgStrongCount;
  st.emgSource = (uint8_t)emgSource;
  st.unlockLevel = unlockLevel;
  st.unlockRatio = unlockRatio;
  st.unlockHMax = unlockHMax;
  st.closeEogNeed = closeEogNeed;
  st.closeEogOff = closeEogOff ? 1 : 0;
  st.unlockBite = unlockBite ? 1 : 0;
  st.biteLevel = biteUnlockLevel;
  st.clenchDur = clenchDurMs;
  st.holdAvg = unlockAvgNeed;
  st.biteAvg = biteAvgNeed;
  st.retFree = retFree ? 1 : 0;
  bool ok = false;
  if (prefs.begin("neurox", false)) {
    ok = prefs.putBytes("train", &st, sizeof(st)) == sizeof(st);
    prefs.end();
  }
  if (!ok) txPrintf(true, "[TRAIN] note: could not save to flash - training is active until the next reboot\n");
}

static void trainForget() {
  if (trainPhase != TRAIN_OFF) trainAbort("u pressed");
  base = FACTORY_BASE;
  emgMinCount = EMG_MIN_COUNT;
  emgStrongCount = EMG_STRONG_COUNT;
  emgSource = EMG_SRC_BOTH;
  unlockLevel = UNLOCK_FACTORY_LEVEL;
  unlockRatio = UNLOCK_FACTORY_RATIO_SET;
  unlockHMax = 0.0f;
  closeEogNeed = 0.0f;
  closeEogOff = false;
  unlockBite = false;
  biteUnlockLevel = 0.0f;
  clenchDurMs = 700;
  applyClenchTiming();
  unlockAvgNeed = 0.0f;
  biteAvgNeed = 0.0f;
  retFree = false;
  personalized = false;
  applyScales();
  if (prefs.begin("neurox", false)) {
    prefs.remove("train");
    prefs.end();
  }
  printThresholds("Factory v13.2 thresholds restored");
}

// =============================================================================
// LR-PRO: anti-false-command filters (added after the competition LR2 version)
// =============================================================================
// The gesture engine (v13.2) is unchanged. These run AFTER it decided:
//  1) TALKING / CHEWING FILTER - a deliberate clench has a quiet jaw before it
//     and exactly 1 bite (FORWARD) or 2 bites (TURN RIGHT). Talking and chewing
//     are a stream of jaw bursts, so their clench commands are dropped.
//     A FORWARD that came with exactly 2 separate bites was a double clench the
//     watcher missed: it becomes TURN RIGHT.
//     TURN RIGHT goes out 0.9 s after the 2nd bite if no 3rd bite follows.
//  2) LOOK-ASIDE / LOOK-DOWN VETO - a blink comes back to where the eye signal
//     was within ~0.3 s; looking down at the car (or up) leaves it shifted.
//     Such a "blink" is not sent as REVERSE.
//  3) SOFT-BLINK FILTER (only after personal training 't') - your 5 training
//     blinks set how firm a command blink is; an everyday blink is softer.
//     A blink under 65 % of your trained blink is not sent as REVERSE / LEFT.
//  4) A clench right after a double-clench turn is no longer swallowed for 2.5 s
//     (SWALLOW_CLENCH_MS 2500 -> 1200).

static const bool     TALK_FILTER_ENABLED    = true;
static const uint16_t TALK_BURST_ON_SAMPLES  = 16;     // jaw EMG above half the FORWARD level for 62 ms = a burst
static const uint16_t TALK_BURST_OFF_SAMPLES = 32;     // below 70 % of that for 125 ms = burst over (FINAL: long bites do not split)
static const float    TALK_ON_FRAC           = 0.50f;  // burst level = 50 % of the FORWARD EMG level ...
static const float    TALK_ON_MIN            = 30.0f;  // ... but never below 30 (resting jaw is ~15)
static const uint32_t TALK_BUSY_MS           = 800;    // jaw already busy this long when a clench is detected = talking
static const uint32_t TALK_LOOKBACK_MS       = 4600;   // 2.5 s quiet before + the gesture itself
static const uint32_t TALK_LONG_BURST_MS     = 1700;   // one jaw burst this long = talking / humming, not a ~1 s bite
static const uint32_t TALK_RIGHT_CONFIRM_MS  = 900;    // TURN RIGHT waits 0.9 s for a 3rd bite (talking)
static const bool     GLANCE_VETO_ENABLED    = true;
static const float    GLANCE_SHIFT_FRAC      = 0.5f;   // REVERSE dropped if the level is still shifted > 50 % of the blink rule
static const bool     FIRM_BLINK_ENABLED     = true;
static const float    FIRM_BLINK_FRAC        = 0.65f;  // trained: a command blink is >= 65 % of your median training blink

struct TalkBurst { uint32_t onMs; uint32_t offMs; bool used; };
static TalkBurst talkB[8];
static uint8_t talkHead = 0;
static bool talkIn = false;
static uint16_t talkOnCnt = 0, talkOffCnt = 0;
static bool rightDefer = false;
static uint32_t rightDeferMs = 0;
static uint32_t talkBlocked = 0, glanceBlocked = 0;

static float talkOnLevel() {
  float l = TALK_ON_FRAC * thr.fwdEmg;
  if (l < TALK_ON_MIN) l = TALK_ON_MIN;
  const float nz = 2.3f * jawNoise() + 7.0f;     // FINAL: a burst must stand out from YOUR resting jaw noise,
  return (l > nz) ? l : nz;                      // or two bites merge into one 'talking' burst
}

// every RUN sample, with the same jaw EMG the gesture engine uses
static void talkSample(float emg) {
  const uint32_t now = (uint32_t)millis();
  if (!talkIn) {
    if (emg > talkOnLevel()) {
      if (++talkOnCnt >= TALK_BURST_ON_SAMPLES) {
        talkIn = true;
        talkOffCnt = 0;
        TalkBurst &b = talkB[talkHead];
        b.onMs = now - (uint32_t)TALK_BURST_ON_SAMPLES * 1000UL / FS_HZ;
        b.offMs = 0;
        b.used = false;
        talkHead = (uint8_t)((talkHead + 1) & 7u);
      }
    } else {
      talkOnCnt = 0;
    }
  } else {
    if (emg < 0.7f * talkOnLevel()) {
      if (++talkOffCnt >= TALK_BURST_OFF_SAMPLES) {
        talkIn = false;
        talkOnCnt = 0;
        talkB[(uint8_t)((talkHead + 7u) & 7u)].offMs = now;
      }
    } else {
      talkOffCnt = 0;
    }
  }
}

static uint8_t talkCount(uint32_t now, uint32_t *longMs) {
  uint8_t n = 0;
  uint32_t lng = 0;
  for (uint8_t i = 0; i < 8; ++i) {
    const TalkBurst &b = talkB[i];
    if (b.onMs == 0 || b.used || (uint32_t)(now - (b.offMs ? b.offMs : now)) > TALK_LOOKBACK_MS) continue;
    ++n;
    const uint32_t d = (b.offMs ? b.offMs : now) - b.onMs;
    if (d > lng) lng = d;
  }
  if (longMs) *longMs = lng;
  return n;
}

// seqGesture(): is the jaw already busy (a burst running for a while) when this clench is seen?
static bool talkBusyNow() {
  if (!talkIn) return false;
  const TalkBurst &b = talkB[(uint8_t)((talkHead + 7u) & 7u)];
  return (uint32_t)((uint32_t)millis() - b.onMs) > TALK_BUSY_MS;
}

static void talkUseAll() {
  for (uint8_t i = 0; i < 8; ++i) talkB[i].used = true;
}

// seqFire(): which command may go out? GEST_NONE = drop it.
static Gesture talkFilter(Gesture g) {
  if (g == GEST_REVERSE || g == GEST_LEFT) talkUseAll();   // its eyelid EMG belongs to the blink
  if (!TALK_FILTER_ENABLED || (g != GEST_FORWARD && g != GEST_RIGHT)) return g;
  const uint32_t now = (uint32_t)millis();
  uint32_t lng = 0;
  const uint8_t n = talkCount(now, &lng);
  const bool busy = (g == GEST_FORWARD) && seqJawBusy;
  seqJawBusy = false;
  if (g == GEST_FORWARD && !busy && n == 2 && lng < talkLongMs) {   // a double clench the watcher missed
    uint32_t a = 0, b = 0;
    for (uint8_t i = 0; i < 8; ++i) {
      const TalkBurst &x = talkB[i];
      if (x.onMs == 0 || x.used || (uint32_t)(now - (x.offMs ? x.offMs : now)) > TALK_LOOKBACK_MS) continue;
      if (a == 0) a = x.onMs; else b = x.onMs;
    }
    const uint32_t gap = (a > b) ? a - b : b - a;
    if (gap >= 300UL && gap <= dblClenchMs) {
      talkUseAll();
      return GEST_RIGHT;
    }
  }
  const uint8_t allowed = (g == GEST_FORWARD) ? 1 : 2;
  if (!busy && n <= allowed && lng < talkLongMs) {
    talkUseAll();                                // accepted: these bites belong to this command
    return g;
  }
  ++talkBlocked;                                 // blocked: the bursts stay, so talking keeps blocking
  txPrintf(true, "[TALK] %s ignored: %s - talking / chewing? (clench on a quiet jaw)\n",
           (g == GEST_FORWARD) ? "FORWARD" : "TURN RIGHT",
           busy ? "jaw was already moving before the clench"
                : (lng >= talkLongMs) ? "one very long jaw burst" : "too many jaw bursts in 4.6 s");
  return GEST_NONE;
}

static void rightDefer_clear() { rightDefer = false; }

static void talkDeferRight() {
  rightDefer = true;
  rightDeferMs = (uint32_t)millis();
}

// every RUN sample: send a deferred TURN RIGHT unless a 3rd bite came
static void talkService() {
  if (!rightDefer) return;
  const uint32_t now = (uint32_t)millis();
  for (uint8_t i = 0; i < 8; ++i) {
    const TalkBurst &b = talkB[i];
    if (b.onMs != 0 && !b.used && (int32_t)(b.onMs - rightDeferMs) > 60) {
      rightDefer = false;
      ++talkBlocked;
      txPrintf(true, "[TALK] TURN RIGHT ignored: a 3rd jaw burst followed - talking / chewing?\n");
      return;
    }
  }
  if ((uint32_t)(now - rightDeferMs) >= TALK_RIGHT_CONFIRM_MS) {
    if (talkIn) {                                // 2nd bite still held: wait until the jaw relaxes ...
      const TalkBurst &cur = talkB[(uint8_t)((talkHead + 7u) & 7u)];
      if ((uint32_t)(now - cur.onMs) < talkLongMs) return;   // ... or the burst is too long (talkFilter drops it)
    }
    rightDefer = false;
    seqFire(GEST_RIGHT);
  }
}

// seqFire() for REVERSE / LEFT after personal training: firm command blink or an everyday blink?
static bool softBlinkVeto(Gesture g) {
  if (!FIRM_BLINK_ENABLED || !personalized || (g != GEST_REVERSE && g != GEST_LEFT)) return false;
  const float need = firmP2P(FIRM_BLINK_FRAC);   // FINAL: % of your median blink, minus room for eye noise
  if (seqP2PBefore >= need) return false;
  ++glanceBlocked;
  txPrintf(true, "[EYE] %s ignored: soft blink (size %.0f, a command blink is %.0f+) - everyday blink\n",
           (g == GEST_LEFT) ? "TURN LEFT" : "REVERSE", (double)seqP2PBefore, (double)need);
  return true;
}

// seqFire() for REVERSE: did the eye signal come back (blink) or stay shifted (looking down/up)?
static bool glanceVeto(Gesture g) {
  if (!GLANCE_VETO_ENABLED || g != GEST_REVERSE) return false;
  const float shift = chV.level - seqLevelBefore;
  if (fabsf(shift) <= glanceLimit()) return false;   // FINAL: allows for electrode drift
  ++glanceBlocked;
  txPrintf(true, "[EYE] REVERSE ignored: eye signal stayed shifted by %+.0f - looking down/up, not a blink\n", (double)shift);
  return true;
}

// =============================================================================
// SAFETY LOCK (NeuroX Safe)
//   LOCKED  : gestures are ignored, the car only ever receives STOP.
//   unlock  : squeeze your eyes shut HARD and hold it for 3 s (forehead muscle
//             signal held above unlockLevel; breaks shorter than 0.12 s allowed).
//   RELEASE : unlocked - waiting for the eyes to relax, then a 1 s grace period
//             (the opening of the eyes is not taken as a blink command).
//   ACTIVE  : blinks / clenches drive the car as before.
//   relock  : 3 s without a command -> LOCKED again.
// Chewing, talking and coughing come in short bursts, so they cannot hold the
// signal for 3 s. Training step 3 measures your own squeeze; untrained the
// factory level is used. Keyboard keys still work in every state (operator).
// =============================================================================
// ---- Buzzer (feedback you can hear with your eyes closed) -------------------
// A 2048 Hz tone = 8 x the 256 Hz sample rate: any supply ripple it causes is
// sampled as a constant offset (0 Hz), which the EOG/EMG filters remove.
static void buzzInit() {
  if (!BUZZER_ENABLED) return;
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
  ledcAttach(PIN_BUZZER, BUZZ_FREQ_HZ, 8);
  ledcWrite(PIN_BUZZER, 0);
#else
  ledcSetup(BUZZ_LEDC_CH, BUZZ_FREQ_HZ, 8);
  ledcAttachPin(PIN_BUZZER, BUZZ_LEDC_CH);
  ledcWrite(BUZZ_LEDC_CH, 0);
#endif
}

static void buzzOut(bool on) {
  if (!BUZZER_ENABLED) return;
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
  ledcWrite(PIN_BUZZER, on ? BUZZ_DUTY : 0);
#else
  ledcWrite(BUZZ_LEDC_CH, on ? BUZZ_DUTY : 0);
#endif
  buzzOnNow = on;
  if (!on) buzzQuietUntilMs = (uint32_t)millis() + BUZZ_ARTIFACT_MS;
}

// Plays a pattern: durations in ms, alternating ON, OFF, ON, ... (non-blocking).
static void buzz(const uint16_t *seq, uint8_t len) {
  if (!BUZZER_ENABLED || len == 0) return;
  buzzSeq = seq;
  buzzLen = len;
  buzzIdx = 0;
  buzzStepMs = (uint32_t)millis();
  buzzOut(true);
}

static void buzzService() {
  if (buzzSeq == nullptr) return;
  const uint32_t now = (uint32_t)millis();
  if ((uint32_t)(now - buzzStepMs) < buzzSeq[buzzIdx]) return;
  buzzStepMs = now;
  if (++buzzIdx >= buzzLen) {
    buzzSeq = nullptr;
    buzzOut(false);
    return;
  }
  buzzOut((buzzIdx & 1u) == 0);
}

// true while the buzzer is on and for 0.1 s after (its electrical step is ignored)
static bool buzzArtifact() {
  return buzzOnNow || (int32_t)(buzzQuietUntilMs - (uint32_t)millis()) > 0;
}

static const char *lockName() {
  switch (lockState) {
    case LOCK_LOCKED:  return "LOCKED";
    case LOCK_RELEASE: return "UNLOCKING";
    case LOCK_ACTIVE:  return "ACTIVE";
  }
  return "?";
}

static void lockEngage(const char *why) {
  lockState = LOCK_LOCKED;
  holdActive = false;
  holdGap = 0;
  holdShownSec = 0;
  seqClear();
  rightDefer = false;                            // PRO: a waiting TURN RIGHT is dropped too
  if (motorPhase == MOTOR_PULSE) motorStopAll();
  else radioSend('S');
  txPrintf(true, "[LOCK] LOCKED (%s) - close your eyes firmly for %lu s to unlock\n", why,
           (unsigned long)(UNLOCK_HOLD_MS / 1000UL));
  buzz(BZ_LOCK, sizeof(BZ_LOCK) / sizeof(BZ_LOCK[0]));
}

// After an unlock the gesture engine first has to settle (your eyes just
// opened). READY is announced - with a short beep - only when a blink or
// clench will really be read; the 3 s lock timer starts from READY.
static bool lockReadyShown = true;
static uint32_t lockActiveSinceMs = 0;
static const uint32_t LOCK_READY_MAX_WAIT_MS = 3000;   // READY at the latest this long after ACTIVE

static void lockActivate(const char *why) {
  lockState = LOCK_ACTIVE;
  talkUseAll();                                  // PRO: the unlock squeeze is not talking - forget it
  lockGraceUntilMs = (uint32_t)millis() + UNLOCK_GRACE_MS;
  lastActivityMs = (uint32_t)millis();
  lockActiveSinceMs = (uint32_t)millis();
  lockReadyShown = false;
  txPrintf(true, "[LOCK] ACTIVE (%s) - eyes open, getting ready... wait for READY\n", why);
}

static void lockReadyService() {
  if (lockState != LOCK_ACTIVE || lockReadyShown) return;
  const uint32_t now = (uint32_t)millis();
  const bool ready = lockAllowsCommands() && gState == ST_ARMED;
  if (!ready && (uint32_t)(now - lockActiveSinceMs) < LOCK_READY_MAX_WAIT_MS) {
    lastActivityMs = now;                        // lock timer paused until READY
    return;
  }
  lockReadyShown = true;
  lastActivityMs = now;
  talkUseAll();                                  // PRO: the unlock squeeze is not talking - forget it
  txPrintf(true, "[LOCK] READY - drive now (clench = forward, blink = reverse); %lu s without a command locks it again\n",
           (unsigned long)(ACTIVE_TIMEOUT_MS / 1000UL));
  buzz(BZ_REC, sizeof(BZ_REC) / sizeof(BZ_REC[0]));
}

static bool lockAllowsCommands() {
  return lockState == LOCK_ACTIVE && (int32_t)((uint32_t)millis() - lockGraceUntilMs) >= 0;
}

static void lockNoteActivity() {
  lastActivityMs = (uint32_t)millis();
}

// Smooths the muscle signals a little more (~0.15 s) for the 3 s hold logic, so
// the forehead/temple comparison does not flicker. Runs on every RUN sample.
static void lockSmooth(float vEmg, float hEmg) {
  lockV += LOCK_SMOOTH_ALPHA * (vEmg - lockV);
  lockH += LOCK_SMOOTH_ALPHA * (hEmg - lockH);
}

// Eyelid-closing movement: closing the eyes moves the lid like the first half
// of a blink, which shows up on the eye signal. Frowning, chewing and talking
// do not. A hold only counts if such a movement happens near its start.
static void lockEyeMove(float vEog) {
  if (buzzArtifact()) return;                    // the buzzer's own electrical step is not an eye movement
  // trained: threshold measured from your own eye closures; untrained: 70 % of the blink trigger.
  // A trained value of 0 means "your closure is not visible on the eye signal" -> check off.
  const float need = (closeEogNeed > 0.0f) ? closeEogNeed : (closeEogOff ? 0.0f : LOCK_CLOSE_EOG_FRAC * thr.trigV);
  if (need <= 0.0f || fabsf(vEog) >= need) lastEyeMoveMs = (uint32_t)millis();
  // training step 3: biggest eye movement at the start of each closure / during rest
  if (trainPhase == TRAIN_SQUEEZE) {
    const int32_t ts = (int32_t)((uint32_t)millis() - sqStartMs);
    if (ts < 0) return;                          // lead-in pause
    const uint32_t t = (uint32_t)ts;
    const float a = fabsf(vEog);
    if (t >= 2700 && t < 4800 && a > sqClose1) sqClose1 = a;
    if (t >= 8700 && t < 10800 && a > sqClose2) sqClose2 = a;
    if (t >= 1000 && t < 2700 && a > sqRestEog) sqRestEog = a;   // clean rest: before any closure
  }
}

// Called on every RUN sample (not during training). vEmg/hEmg = the normal
// muscle envelopes (0.125 s), used for the level, so pauses in speech or
// chewing break the hold; lockV/lockH (extra 0.15 s smoothing) for the ratio.
static void lockSample(float vEmg, float hEmg) {
  if (trainPhase != TRAIN_OFF) return;
  const uint32_t now = (uint32_t)millis();
  const bool clipped = satV || vOnRail;
  const float unlockLvl = effUnlockLevel();      // FINAL: never inside the resting noise
  // Forehead signal high AND (if training found it useful) forehead-dominant:
  // a squeeze is mostly forehead, a clench mostly temples - at any effort level.
  // Starting a hold needs the full level; KEEPING it only needs 80 % of it, so a
  // small dip while your eyes are closed does not break the hold.
  const float keepLevel = UNLOCK_KEEP_FRAC * unlockLvl;
  const float needNow = (holdActive && closureSeen) ? keepLevel : unlockLvl;
  const bool squeezeEyes = !clipped && (vEmg > needNow) && (lockV >= unlockRatio * lockH) &&
                           (unlockHMax <= 0.0f || lockH < unlockHMax);
  // FINAL: bite-and-hold unlock, only when calibration found the eye squeeze too weak
  const float biteNeed = (holdActive && holdByBite) ? UNLOCK_KEEP_FRAC * effBiteLevel() : effBiteLevel();
  const bool biteHold = unlockBite && !satH && (hEmg > biteNeed);
  const bool squeeze = squeezeEyes || biteHold;

  if (lockState == LOCK_LOCKED) {
    // Live "waiting" line (2 per second) whenever the forehead is clearly active but a
    // hold is not counting yet - so you always see what the lock sees.
    if (!holdTicked && lockV > LOCK_METER_FRAC * unlockLvl && (uint32_t)(now - lastMeterMs) >= LOCK_METER_MS) {
      lastMeterMs = now;
      const bool eyesSeen = holdActive ? closureSeen : ((uint32_t)(now - lastEyeMoveMs) <= LOCK_CLOSE_BEFORE_MS);
      txPrintf(true, "[LOCK] waiting  [......]  forehead %.0f (need > %.0f) | eyes closing seen: %s%s\n",
               (double)vEmg, (double)unlockLvl, eyesSeen ? "YES" : "no",
               (vEmg > unlockLvl && !eyesSeen) ? " - open, then close your eyes in one movement" : "");
    }
    // Feedback: something squeeze-like is happening but it does not count yet.
    const bool partly = !clipped && !squeeze && (vEmg > LOCK_HINT_FRAC * unlockLvl);
    if (!partly) partlySinceMs = now;              // hint only after 0.4 s (skips a squeeze's ramp-up)
    if (partly && (uint32_t)(now - partlySinceMs) >= 400UL && (uint32_t)(now - lastHintMs) >= 1000UL) {
      lastHintMs = now;
      buzz(BZ_WEAK, sizeof(BZ_WEAK) / sizeof(BZ_WEAK[0]));   // "seen, but not counting"
      if (unlockHMax > 0.0f && lockH >= unlockHMax && vEmg > unlockLvl) {
        txPrintf(true, "[LOCK] temple signal too high (%.0f, limit %.0f) - close your eyes a little gentler, like in training, jaw relaxed\n",
                 (double)lockH, (double)unlockHMax);
      } else if (vEmg <= unlockLvl) {
        txPrintf(true, "[LOCK] eyes closed but too weak: forehead %.0f, need > %.0f - close a little more firmly (or press NEW to calibrate again)\n",
                 (double)vEmg, (double)unlockLvl);
      } else {
        txPrintf(true, "[LOCK] squeeze seen but it looks like a jaw clench: forehead %.0f / temple %.0f = %.1f, need >= %.1f - relax the jaw\n",
                 (double)lockV, (double)lockH, (double)(lockV / (lockH > 1.0f ? lockH : 1.0f)), (double)unlockRatio);
      }
    }
    if (clipped && (uint32_t)(now - lastHintMs) >= 1000UL && holdActive) {
      lastHintMs = now;
      txPrintf(true, "[LOCK] forehead signal clipped (electrode / too much movement) - squeeze a little less hard\n");
    }
    if (squeeze) {
      if (!holdActive) {
        holdActive = true;
        holdStartMs = now;
        holdShownSec = 0;
        holdTicked = false;
        closureSeen = (uint32_t)(now - lastEyeMoveMs) <= LOCK_CLOSE_BEFORE_MS;
        holdByBite = biteHold && !squeezeEyes;
        holdSum = 0.0f;
        holdN = 0;
        if (holdByBite) closureSeen = true;      // a bite-and-hold needs no eye closing
        noCloseHinted = false;
      }
      holdGap = 0;
      const uint32_t held = now - holdStartMs;
      if (closureSeen) { holdSum += holdByBite ? hEmg : vEmg; ++holdN; }
      if (!closureSeen && held <= LOCK_CLOSE_AFTER_MS && lastEyeMoveMs == now) closureSeen = true;
      if (!closureSeen && held > LOCK_CLOSE_AFTER_MS) {
        // muscle held, but the eyes never closed: frown / chewing / talking - not an unlock
        if (!noCloseHinted && held >= 1500UL) {
          noCloseHinted = true;
          txPrintf(true, "[LOCK] forehead held, but no eye closing seen - close your EYES (frowning or chewing does not unlock)\n");
          buzz(BZ_BROKEN, sizeof(BZ_BROKEN) / sizeof(BZ_BROKEN[0]));
        }
        return;
      }
      if (!closureSeen) return;                    // still waiting (0.6 s) for the eye-closing movement
      if (!holdTicked && held < HOLD_TICK_DELAY_MS) return;   // ignore flickers shorter than 0.3 s
      if (!holdTicked) {                           // a real hold has started: first tick
        holdTicked = true;
        buzz(BZ_TICK, sizeof(BZ_TICK) / sizeof(BZ_TICK[0]));
        txPrintf(true, "[LOCK] holding 0.0 / %.1f s  [......]  forehead %.0f (keep > %.0f) - keep your eyes closed\n",
                 (double)UNLOCK_HOLD_MS / 1000.0, (double)vEmg, (double)keepLevel);
      }
      const float avgNeed = holdByBite ? biteAvgNeed : unlockAvgNeed;
      if (held >= UNLOCK_HOLD_MS && avgNeed > 0.0f && holdN > 0 && holdSum / (float)holdN < avgNeed) {
        // FINAL: held for 3 s, but far weaker than your squeeze: a yawn / a squint, not an unlock
        holdActive = false;
        buzz(BZ_BROKEN, sizeof(BZ_BROKEN) / sizeof(BZ_BROKEN[0]));
        txPrintf(true, "[LOCK] held 3 s but too gently (%s average %.0f, needs %.0f) - a yawn does not unlock; %s\n",
                 holdByBite ? "temple" : "forehead", (double)(holdSum / (float)holdN), (double)avgNeed,
                 holdByBite ? "BITE firmly" : "close your eyes FIRMLY");
        return;
      }
      if (held >= UNLOCK_HOLD_MS) {
        holdActive = false;
        lockState = LOCK_RELEASE;
        releaseCount = 0;
        seqClear();
        releaseStartMs = now;
        txPrintf(true, "[LOCK] UNLOCKED - now relax%s\n", holdByBite ? " your jaw" : " and open your eyes");
        buzz(BZ_UNLOCK, sizeof(BZ_UNLOCK) / sizeof(BZ_UNLOCK[0]));
      } else if (held / 500UL > holdShownSec) {    // progress every 0.5 s, a tick every second
        holdShownSec = (uint8_t)(held / 500UL);
        static const char *const bar[6] = {"[#.....]", "[##....]", "[###...]", "[####..]", "[#####.]", "[######]"};
        txPrintf(true, "[LOCK] holding %.1f / %.1f s  %s  forehead %.0f (keep > %.0f)\n", (double)held / 1000.0,
                 (double)UNLOCK_HOLD_MS / 1000.0, bar[holdShownSec > 5 ? 5 : holdShownSec - 1], (double)vEmg,
                 (double)keepLevel);
        if ((holdShownSec & 1u) == 0) buzz(BZ_TICK, sizeof(BZ_TICK) / sizeof(BZ_TICK[0]));
      }
    } else if (holdActive && ++holdGap > UNLOCK_GAP_SAMPLES) {
      if (now - holdStartMs >= 1000UL && holdTicked) {
        buzz(BZ_BROKEN, sizeof(BZ_BROKEN) / sizeof(BZ_BROKEN[0]));
        txPrintf(true, "[LOCK] hold broken after %.1f s (forehead %.0f / keep >%.0f, temple %.0f, ratio %.1f / need >=%.1f) - squeeze again\n",
                 (double)(now - holdStartMs) / 1000.0, (double)vEmg, (double)keepLevel, (double)lockH,
                 (double)(lockV / (lockH > 1.0f ? lockH : 1.0f)), (double)unlockRatio);
      }
      holdActive = false;
      holdGap = 0;
    }
    return;
  }

  if (lockState == LOCK_RELEASE) {
    // Wait until the squeeze is fully released, then reset the gesture engine
    // so the eye-opening is not read as a blink, and start the grace period.
    const float relLvl = fmaxf(UNLOCK_RELEASE_FRAC * unlockLvl, 1.4f * nzEmgV.q + 4.0f);
    const bool relaxed = holdByBite ? (hEmg < 0.5f * effBiteLevel()) : (vEmg < relLvl);
    if ((!clipped && relaxed) || (uint32_t)(now - releaseStartMs) >= UNLOCK_RELEASE_MAX_MS) {
      if (++releaseCount >= UNLOCK_RELEASE_SAMPLES || (uint32_t)(now - releaseStartMs) >= UNLOCK_RELEASE_MAX_MS) {
        preTriggerClear();
        biteLatched = false;
        if (holdoffSamples < CLIP_HOLDOFF_SAMPLES) holdoffSamples = CLIP_HOLDOFF_SAMPLES;
        enterSettle();
        seqClear();
        lockActivate(holdByBite ? "unlocked by 3 s bite" : "unlocked by 3 s hard blink");
      }
    } else {
      releaseCount = 0;
    }
  }
}

// Called from loop(): 3 s without a command -> LOCKED; reminder while LOCKED.
static void lockService() {
  if (lockState == LOCK_LOCKED && trainPhase == TRAIN_OFF && sysMode == MODE_RUN && !holdActive &&
      (uint32_t)((uint32_t)millis() - lastReminderMs) >= LOCK_REMINDER_MS) {
    lastReminderMs = (uint32_t)millis();
    txPrintf(true, "[LOCK] still LOCKED - close your eyes firmly for %lu s (unlock level %.0f%s) | o = unlock by key\n",
             (unsigned long)(UNLOCK_HOLD_MS / 1000UL), (double)unlockLevel, personalized ? "" : ", not calibrated: press NEW");
  }
  if (lockState != LOCK_ACTIVE || trainPhase != TRAIN_OFF) return;
  lockReadyService();
  if (!lockReadyShown) return;
  if (motorPhase != MOTOR_IDLE || seqPending != GEST_NONE || seqQueued != GEST_NONE || rightDefer) return;
  if (gState == ST_CAPTURE) return;              // FINAL: a gesture is being measured right now (0.2 s)
  if ((uint32_t)((uint32_t)millis() - lastActivityMs) >= ACTIVE_TIMEOUT_MS) {
    lockEngage("no command for 3 s");
  }
}

static void lockToggleByKey() {
  if (lockState == LOCK_LOCKED) {
    seqClear();
    lockState = LOCK_ACTIVE;
    lockReadyShown = true;                       // keyboard unlock: ready at once
    lockGraceUntilMs = (uint32_t)millis();
    lastActivityMs = (uint32_t)millis();
    txPrintf(true, "[LOCK] ACTIVE (o key) - %lu s without a command locks it again\n",
             (unsigned long)(ACTIVE_TIMEOUT_MS / 1000UL));
  } else {
    lockEngage("o key");
  }
}

// ---- Training step 3: measure the unlock squeeze ---------------------------
// Firmware-timed: relax 3 s, squeeze 3 s, relax 3 s, squeeze 3 s. Records the
// forehead / temple muscle signals and sets the unlock level at half of your
// squeeze. If your squeeze is clearly more forehead-dominant than your clench,
// a forehead/temple ratio is required too, so a clench cannot unlock.
static float pctile(float *v, uint16_t n, float frac) {
  if (n == 0) return 0.0f;
  uint16_t k = (uint16_t)(frac * (float)(n - 1));
  std::nth_element(v, v + k, v + n);
  return v[k];
}

static void squeezeStart() {
  trainPhase = TRAIN_SQUEEZE;
  sqStartMs = (uint32_t)millis() + SQ_LEAD_IN_MS;   // countdown starts after a short pause
  sqLastSec = 0xFF;
  sqRestN = sqSqueezeN = 0;
  sqClose1 = sqClose2 = sqRestEog = 0.0f;
  txPrintf(true, "\n[TRAIN] Step 3/3: UNLOCK gesture - relax your jaw, countdown starts in %.1f s (12 s):\n",
           (double)SQ_LEAD_IN_MS / 1000.0);
  txPrintf(true, "        relax 3 s -> close eyes FIRMLY 3 s -> relax 3 s -> close FIRMLY again 3 s\n");
  txPrintf(true, "        Use the same firm closure you want to unlock with - it does NOT need to be maximum.\n");
}

// 0-3 s relax, 3-6 s squeeze, 6-9 s relax, 9-12 s squeeze (first 0.5 s of each squeeze skipped).
static void squeezeSample(float vEmg, float hEmg) {    // raw envelopes; ratio uses lockV/lockH
  if (trainPhase != TRAIN_SQUEEZE) return;
  const int32_t ts = (int32_t)((uint32_t)millis() - sqStartMs);
  if (ts < 0) return;                            // lead-in pause
  const uint32_t t = (uint32_t)ts;
  // FINAL: 'relaxed' only before the 1st closure - a slow reaction pushes the 1st
  // closure into the 2nd relax window, which then looked like a high resting level
  const bool rest = (t >= 1000 && t < 3000);
  const bool sq = (t >= 3500 && t < 6000) || (t >= 9500 && t < 12000);
  if (rest && sqRestN < SQ_REST_MAX) {
    sqRestV[sqRestN] = vEmg;
    sqRestH[sqRestN] = hEmg;
    ++sqRestN;
  } else if (sq && sqSqueezeN < SQ_SQUEEZE_MAX) {
    sqSqzV[sqSqueezeN] = vEmg;
    sqSqzH[sqSqueezeN] = lockV / (lockH > 1.0f ? lockH : 1.0f);   // smoothed forehead/temple ratio
    sqSqzT[sqSqueezeN] = lockH;                                   // smoothed temple signal
    ++sqSqueezeN;
  }
}

static void squeezeFinish() {
  static float vTmp[SQ_SQUEEZE_MAX];               // static: keeps 5 KB off the loop task's stack
  // relaxed level (90th percentile), squeeze level (25th = what you hold most of the time)
  for (uint16_t i = 0; i < sqRestN; ++i) vTmp[i] = sqRestV[i];
  const float restV = pctile(vTmp, sqRestN, 0.90f);
  for (uint16_t i = 0; i < sqSqueezeN; ++i) vTmp[i] = sqSqzV[i];
  const float sqzV = pctile(vTmp, sqSqueezeN, SQ_HOLD_PCT);   // FINAL: 0.25 -> 0.45 (a late start leaves the window's beginning empty)
  for (uint16_t i = 0; i < sqSqueezeN; ++i) vTmp[i] = sqSqzH[i];
  const float sqzRatio = pctile(vTmp, sqSqueezeN, SQ_HOLD_PCT);    // forehead/temple, conservative-low
  for (uint16_t i = 0; i < sqSqueezeN; ++i) vTmp[i] = sqSqzT[i];
  const float sqzH75 = pctile(vTmp, sqSqueezeN, 0.75f);           // temple signal while squeezing

  // typical temple / forehead signal of your clenches (step 2)
  float cH[TRAIN_SAMPLES], cV[TRAIN_SAMPLES], cR[TRAIN_SAMPLES];
  for (uint8_t i = 0; i < TRAIN_SAMPLES; ++i) {
    const TrainSample &c = trainClench[i];
    uint16_t n = c.traceLen;
    for (uint16_t k = 0; k < n; ++k) vTmp[k] = c.traceV[k] / (c.traceH[k] > 1.0f ? c.traceH[k] : 1.0f);
    cR[i] = pctile(vTmp, n, 0.75f);              // forehead/temple ratio, conservative-high
    for (uint16_t k = 0; k < n; ++k) vTmp[k] = c.traceH[k];
    cH[i] = pctile(vTmp, n, 0.75f);              // plateau of the clench, not its ramp-up
    for (uint16_t k = 0; k < n; ++k) vTmp[k] = c.traceV[k];
    cV[i] = pctile(vTmp, n, 0.75f);
  }
  const float clenchH = median5(cH);
  const float clenchV = median5(cV);
  float cRs[TRAIN_SAMPLES];
  for (uint8_t i = 0; i < TRAIN_SAMPLES; ++i) cRs[i] = cR[i];
  float clenchRatio = 0.0f;                      // highest (2nd highest, robust) clench ratio
  { float hi1 = 0.0f, hi2 = 0.0f;
    for (uint8_t i = 0; i < TRAIN_SAMPLES; ++i) { if (cRs[i] > hi1) { hi2 = hi1; hi1 = cRs[i]; } else if (cRs[i] > hi2) hi2 = cRs[i]; }
    clenchRatio = hi2; }

  trainPhase = TRAIN_OFF;
  // FINAL: bite-and-hold unlock level (used when the squeeze is too weak): half of your clench on the temples
  for (uint16_t i = 0; i < sqRestN; ++i) vTmp[i] = sqRestH[i];
  const float restH = pctile(vTmp, sqRestN, 0.90f);
  biteUnlockLevel = BITE_UNLOCK_FRAC * clenchH;
  biteAvgNeed = BITE_AVG_FRAC * clenchH;
  if (biteUnlockLevel < 2.5f * restH) biteUnlockLevel = 2.5f * restH;
  if (biteUnlockLevel < UNLOCK_MIN_LEVEL) biteUnlockLevel = UNLOCK_MIN_LEVEL;
  if (biteUnlockLevel > UNLOCK_MAX_SQZ_FRAC * clenchH) biteUnlockLevel = UNLOCK_MAX_SQZ_FRAC * clenchH;   // FINAL: reachable
  const bool good = (sqzV >= UNLOCK_MIN_RATIO * restV) && (sqzV >= UNLOCK_MIN_LEVEL) &&
                    (UNLOCK_MAX_SQZ_FRAC * sqzV >= UNLOCK_SEP_REST * restV);   // FINAL: must stand out from rest (tremor)
  if (!good) {
    if (acPhase == AC_SQUEEZE) {
      txPrintf(true, "[TRAIN] unlock step: squeeze %.0f vs relaxed %.0f - not clear enough\n", (double)sqzV, (double)restV);
      acComplete(false, false);
      return;
    }
    txPrintf(true, "[TRAIN] unlock step: squeeze signal too weak (%.0f vs relaxed %.0f) - kept the previous unlock level %.0f\n",
             (double)sqzV, (double)restV, (double)unlockLevel);
    txPrintf(true, "        Close your eyes a bit more firmly (tighten them), then press t to train again.\n");
    trainSave();
    lockEngage("training finished");
    return;
  }
  // Unlock level: 70 % of your sustained squeeze, never harder than the level
  // you chose (UNLOCK_FACTORY_LEVEL_SET = 30), but always well above relaxed.
  float level = UNLOCK_SQUEEZE_FRAC * sqzV;
  if (level > UNLOCK_FACTORY_LEVEL_SET) level = UNLOCK_FACTORY_LEVEL_SET;
  if (level < UNLOCK_REST_MULT * restV) level = UNLOCK_REST_MULT * restV;
  if (level < UNLOCK_MIN_LEVEL) level = UNLOCK_MIN_LEVEL;
  const float sqzCap = UNLOCK_MAX_SQZ_FRAC * sqzV;   // FINAL: never above 60 % of your squeeze -> always reachable
  if (level > sqzCap) {
    level = sqzCap;                              // the cap wins over every floor (>= 1.3 x relaxed, see `good`)
    txPrintf(true, "[TRAIN] unlock: gentle squeeze - unlock level set to %.0f (60 %% of your squeeze %.0f)\n",
             (double)level, (double)sqzV);
  }
  unlockLevel = level;
  unlockBite = false;
  unlockAvgNeed = UNLOCK_AVG_FRAC * sqzV;        // FINAL: anti-yawn (a yawn's forehead signal is much weaker)
  // Ratio rule: if your squeeze is clearly more forehead-dominant than your
  // clench, require a forehead/temple ratio between the two - a clench (even
  // a strong one) then cannot unlock, and squeezing harder still works.
  const float cRatio = (clenchRatio > 0.05f) ? clenchRatio : 0.05f;
  if (sqzRatio >= UNLOCK_RATIO_SEP * cRatio) {
    // just above your clench (a very hard squeeze clips the forehead signal
    // and lowers its ratio, so the limit must not sit near the squeeze)
    float r = UNLOCK_RATIO_MARGIN * cRatio;
    const float mid = sqrtf(sqzRatio * cRatio);
    if (r > mid) r = mid;
    unlockRatio = r;
  } else {
    unlockRatio = 0.0f;
  }
  // Temple ceiling: when the ratio cannot separate them (your clench also
  // shows strongly on the forehead), a clench is still far stronger on the
  // temples than a comfortable squeeze - cap the temple signal between them.
  const float sqzHref = (sqzH75 > 1.0f) ? sqzH75 : 1.0f;
  unlockHMax = (unlockRatio <= 0.0f && clenchH >= UNLOCK_TEMPLE_SEP * sqzHref) ? sqrtf(sqzHref * clenchH) : 0.0f;
  // Eyelid check: your smaller closure movement (of the two), at half size; off if it is not clearly
  // bigger than what the eye signal does at rest.
  const float closeMove = (sqClose1 < sqClose2) ? sqClose1 : sqClose2;
  if (closeMove >= LOCK_CLOSE_VS_REST * (sqRestEog > 1.0f ? sqRestEog : 1.0f) && closeMove >= LOCK_CLOSE_MIN) {
    closeEogNeed = LOCK_CLOSE_TRAIN_FRAC * closeMove;
    if (closeEogNeed < LOCK_CLOSE_REST_MULT * sqRestEog) closeEogNeed = LOCK_CLOSE_REST_MULT * sqRestEog;
    closeEogOff = false;
  } else {
    closeEogNeed = 0.0f;
    closeEogOff = true;
  }
  if (acPhase != AC_SQUEEZE) trainSave();
  txPrintf(true, "[TRAIN] unlock: forehead signal relaxed %.0f, squeezed %.0f -> unlock above %.0f held %lu s\n",
           (double)restV, (double)sqzV, (double)unlockLevel, (unsigned long)(UNLOCK_HOLD_MS / 1000UL));
  txPrintf(true, "[TRAIN] unlock: forehead/temple ratio squeezed %.1f, clenched %.1f (temple %.0f / %.0f, forehead clenched %.0f)\n",
           (double)sqzRatio, (double)clenchRatio, (double)sqzH75, (double)clenchH, (double)clenchV);
  if (unlockRatio > 0.0f) {
    txPrintf(true, "[TRAIN] unlock: forehead must also be >= %.1fx temple - a clench cannot unlock\n", (double)unlockRatio);
  } else if (unlockHMax > 0.0f) {
    txPrintf(true, "[TRAIN] unlock: temple signal must stay below %.0f (squeeze %.0f, clench %.0f) - a clench cannot unlock\n",
             (double)unlockHMax, (double)sqzH75, (double)clenchH);
  } else if (clenchV > unlockLevel) {
    txPrintf(true, "[TRAIN] note: a 3 s jaw clench may also unlock (your clench looks like your squeeze) - still a deliberate act\n");
  }
  if (!closeEogOff) {
    txPrintf(true, "[TRAIN] eyelid check: your eye closing moves the eye signal by %.0f (rest %.0f) -> needs >= %.0f at the start of a hold\n",
             (double)closeMove, (double)sqRestEog, (double)closeEogNeed);
  } else {
    txPrintf(true, "[TRAIN] eyelid check OFF: your eye closing is not clearly visible on the eye signal (%.0f vs rest %.0f)\n",
             (double)closeMove, (double)sqRestEog);
    txPrintf(true, "        Unlock still needs the 3 s firm closure; long frowns could also unlock. Close your eyes more briskly and train again.\n");
  }
  if (acPhase == AC_SQUEEZE) {                   // FINAL: auto-calibration judges, saves and locks
    acComplete(true, false);
    return;
  }
  txPrintf(true, "[TRAIN] ===== TRAINING COMPLETE (saved) =====\n");
  lockEngage("training finished");
  buzz(BZ_UNLOCK, sizeof(BZ_UNLOCK) / sizeof(BZ_UNLOCK[0]));                      // "done" melody
}

static void squeezeService() {
  if (trainPhase != TRAIN_SQUEEZE) return;
  const int32_t ts = (int32_t)((uint32_t)millis() - sqStartMs);
  if (ts < 0) return;                            // lead-in pause
  const uint32_t t = (uint32_t)ts;
  const uint8_t sec = (uint8_t)(t / 1000UL);
  if (sec != sqLastSec && sec < 12) {
    sqLastSec = sec;
    static const char *const msg[12] = {
      "relax, eyes open ... 3", "relax ... 2", "relax ... 1",
      ">>> close your eyes FIRMLY and hold (comfortable, not max) ... 3", ">>> hold ... 2", ">>> hold ... 1",
      "relax, eyes open ... 3", "relax ... 2", "relax ... 1",
      ">>> close FIRMLY again and hold ... 3", ">>> hold ... 2", ">>> hold ... 1"};
    txPrintf(true, "[TRAIN] %s\n", msg[sec]);
    if (acPhase == AC_SQUEEZE) {                 // FINAL: plain prompts for the auto-calibration
      if (sec == 3 || sec == 9) txPrintf(true, "[AUTO] CLOSE your eyes firmly and hold (%u/2)\n", (sec == 3) ? 1u : 2u);
      if (sec == 6) txPrintf(true, "[AUTO] relax, eyes open\n");
    }
    if (sec == 3 || sec == 9) buzz(BZ_CLOSE, sizeof(BZ_CLOSE) / sizeof(BZ_CLOSE[0]));   // close now
    if (sec == 6) buzz(BZ_RELAX, sizeof(BZ_RELAX) / sizeof(BZ_RELAX[0]));                // open / relax
  }
  if (t >= 12000UL) {
    txPrintf(true, "[TRAIN] relax.\n");
    squeezeFinish();
  }
}

// =============================================================================
// NEUROX FINAL: auto-calibration at power-on, noise tracking, buttons
//   AUTO-CALIBRATION (no keyboard needed) - runs by itself at every power-on,
//   and again whenever the NEW button (GPIO 7) or BOOT (hold 1 s) is pressed:
//     sit still 2 s -> BLINK at each beep (5x) -> CLENCH at each beep (5x)
//     -> close your eyes FIRMLY at the beep, relax at the double beep (2x).
//   Only what you do right after a beep is recorded, so noise and everyday
//   blinks in between cannot spoil it. The strongest response in each window
//   is kept. Bad result -> it asks once more -> still bad -> it keeps your
//   last good calibration (or the factory settings).
//   Squeeze too weak (e.g. facial palsy) -> unlock by BITING AND HOLDING 3 s.
//   NOISE TRACKING: the resting noise of both channels is followed all the
//   time; every threshold and the unlock level stay above it, so loose or
//   noisy electrodes cannot trigger commands or block the unlock.
// =============================================================================
static uint16_t nzArmedRun = 0;
static bool acRestReset = false;
static float acRestEog[AC_REST_BUF], acRestLvl[AC_REST_BUF], acRestEV[AC_REST_BUF], acRestEH[AC_REST_BUF];
static uint16_t acRestN = 0;
static void acRestApply() {                      // exact noise percentiles of the 'sit still' period
  if (acRestN < 256) return;                     // too little: keep tracking as before
  nzEog.q = pctile(acRestEog, acRestN, nzEog.p);
  nzLvl.q = pctile(acRestLvl, acRestN, nzLvl.p);
  nzEmgV.q = pctile(acRestEV, acRestN, nzEmgV.p);
  nzEmgH.q = pctile(acRestEH, acRestN, nzEmgH.p);
  nzEog.n = nzLvl.n = nzEmgV.n = nzEmgH.n = NZ_FAST_SAMPLES;   // from now on: slow tracking
}                  // samples since the engine re-armed
static void noiseTrack(float vEog, float vEmg, float hEmg) {
  // the tail of a bite / blink right after re-arming is not noise either: only count
  // the signal once the engine has been armed and calm for 1 s (without this the
  // floor crept up after every clench and lifted the blink trigger above the blinks)
  if (acPhase == AC_REST) {                      // auto-calibration 'sit still': everything is noise
    const uint32_t el = (uint32_t)millis() - acPhaseMs;
    if (el < AC_REST_SETTLE_MS) return;          // still relaxing from before
    if (acRestN == 0 && gState != ST_ARMED && el < AC_REST_SETTLE_MS + AC_REST_WAIT_MS) return;   // wait for a calm signal
    if (acRestReset) { acRestReset = false; acRestN = 0; }
    if (satV || vOnRail || acRestN >= AC_REST_BUF) return;
    acRestEog[acRestN] = fabsf(vEog);           // kept, and measured exactly at the end of the rest
    acRestLvl[acRestN] = fabsf(chV.level - levelBase);
    acRestEV[acRestN] = vEmg;
    acRestEH[acRestN] = satH ? 0.0f : hEmg;
    ++acRestN;
    return;
  }
  if (gState != ST_ARMED) { nzArmedRun = 0; return; }
  if (nzArmedRun < NZ_ARMED_SETTLE) { ++nzArmedRun; return; }
  // only the calm, armed signal counts as "noise": never a gesture, its settle,
  // a held bite, the unlock squeeze or its release (those pulled the floor up to
  // blink size and made the next blinks invisible)
  if ((trainPhase != TRAIN_OFF && acPhase != AC_REST) || gState != ST_ARMED || biteLatched || holdoffSamples > 0) return;
  if (holdActive || lockState == LOCK_RELEASE) return;
  if (satV || vOnRail) return;
  nzEog.add(fabsf(vEog));
  nzLvl.add(fabsf(chV.level - levelBase));
  nzEmgV.add(vEmg);
  if (!satH) nzEmgH.add(hEmg);
}

static void applyNoiseFloors() {
  if (!NOISE_FLOORS_ENABLED || nzEog.n < NZ_MIN_SAMPLES) return;
  const float e = nzEog.q;
  const float ev = nzEmgV.q, eh = nzEmgH.q;
  const float comb = (emgSource == EMG_SRC_TEMPLES) ? eh : (emgSource == EMG_SRC_FOREHEAD) ? ev : fmaxf(ev, eh);
  thr.quietV = fmaxf(thr.quietV, NZ_QUIET_V_K * e);
  thr.trigV = fmaxf(thr.trigV, NZ_TRIG_K * e);
  thr.quietEmg = fmaxf(thr.quietEmg, NZ_QUIET_EMG_K * comb + 5.0f);
  thr.fwdEmg = fmaxf(thr.fwdEmg, NZ_FWD_EMG_K * comb + 8.0f);
  // slow-bite trigger (no sharp onset): must be bigger than the electrode drift,
  // or drift opens a capture just before a blink and the blink is lost
  if (nzLvl.n >= NZ_MIN_SAMPLES) thr.biteLevel = fmaxf(thr.biteLevel, NZ_DC_K * nzLvl.q);
}

static float jawNoise() {
  if (!NOISE_FLOORS_ENABLED || nzEmgV.n < NZ_MIN_SAMPLES) return 0.0f;
  return (emgSource == EMG_SRC_TEMPLES) ? nzEmgH.q : (emgSource == EMG_SRC_FOREHEAD) ? nzEmgV.q : fmaxf(nzEmgV.q, nzEmgH.q);
}

// REVERSE is vetoed when the eye level stays shifted (looking down/up) - but a
// drifting electrode also shifts it, so the limit grows with the measured wander
static float glanceLimit() {
  const float l = GLANCE_SHIFT_FRAC * thr.biVMax;
  if (!NOISE_FLOORS_ENABLED || nzLvl.n < NZ_MIN_SAMPLES) return l;
  return fmaxf(l, 2.2f * nzLvl.q);
}

// double blink: is the 1st blink clearly smaller than the 2nd (= an everyday blink
// just before a command blink)? With tremor / noise the sizes wobble, so the
// difference must also stand out from the measured eye noise.
static bool blinkPairUneven(float first, float second) {
  if (first >= BLINK_PAIR_FRAC * second) return false;
  const float nz = (NOISE_FLOORS_ENABLED && nzEog.n >= NZ_MIN_SAMPLES) ? nzEog.q : 0.0f;
  return (second - first) > BLINK_PAIR_NOISE_K * nz;
}

// slow blinks: the normal onset rule wants a sharp edge (39 ms); a slow blink of a
// tired / elderly / myasthenic face rises more slowly but just as far
static float slowRing[SLOW_RING_LEN];
static uint8_t slowHead = 0, slowN = 0;
static void slowRingPush(float v) {
  slowRing[slowHead] = v;
  slowHead = (uint8_t)((slowHead + 1) % SLOW_RING_LEN);
  if (slowN < SLOW_RING_LEN) ++slowN;
}
static float slowRingMin() {
  float m = 1.0e9f;
  for (uint8_t i = 0; i < slowN; ++i) if (slowRing[i] < m) m = slowRing[i];
  return (slowN > 0) ? m : 0.0f;
}
static bool slowBlinkOnset(float v) {
  if (slowN < SLOW_RING_LEN) return false;
  return (v >= SLOW_BLINK_TRIG_K * thr.trigV) && (v - slowRingMin() >= thr.trigV);
}

// a blink right after another blink (or out of a bite's undershoot) starts low, so
// its peak may stay under the trigger although it is just as big: a rise of
// 1.5 x the trigger within 125 ms, ending at >= 60 % of it, counts as an onset
static bool dipRiseOnset(float v) {
  if (slowN < SLOW_RING_LEN || v < DIP_RISE_END_K * thr.trigV) return false;
  const float lo = slowRingMin();
  return (lo <= -0.5f * thr.trigV) && (v - lo) >= DIP_RISE_K * thr.trigV;   // really out of a dip
}

// 'firm blink' limits (everyday blinks are smaller): frac x your calibrated blink, but
// with a shaky eye signal (tremor, nystagmus) the same blink measures +-3x the noise
static float eyeNoise() { return (NOISE_FLOORS_ENABLED && nzEog.n >= NZ_MIN_SAMPLES) ? nzEog.q : 0.0f; }
static float firmP2P(float frac) {
  const float med = thr.biP2P / TRAIN_BLINK_FRAC;     // your median blink swing
  float need = frac * med;
  const float loose = med - FIRM_NOISE_K * eyeNoise();
  if (loose < need) need = loose;
  if (need < 0.5f * med) need = 0.5f * med;
  return need;
}
static float firmPeak(float frac) {
  const float med = thr.biVMax / TRAIN_BLINK_FRAC;    // your median blink peak
  float need = frac * med;
  const float loose = med - 1.5f * eyeNoise();
  if (loose < need) need = loose;
  if (need < 0.5f * med) need = 0.5f * med;
  return need;
}

static float effUnlockLevel() {
  float l = fmaxf(unlockLevel, NZ_UNLOCK_K * nzEmgV.q + 6.0f);
  // never above 60 % of your calibrated squeeze: the unlock must always stay reachable
  if (unlockAvgNeed > 0.0f) {
    const float cap = UNLOCK_MAX_SQZ_FRAC * (unlockAvgNeed / UNLOCK_AVG_FRAC);
    if (l > cap) l = cap;                        // strict: the cap wins (also over an older saved level)
  }
  return l;
}
static float effBiteLevel() {
  const float lvl = (biteUnlockLevel > 0.0f) ? biteUnlockLevel : thr.fwdEmg;
  float l = fmaxf(lvl, NZ_UNLOCK_K * nzEmgH.q + 8.0f);
  // never above 60 % of your calibrated clench: bite-and-hold must stay reachable too
  if (biteAvgNeed > 0.0f) {
    const float cap = UNLOCK_MAX_SQZ_FRAC * (biteAvgNeed / BITE_AVG_FRAC);
    if (l > cap) l = cap;
  }
  return l;
}

// double-clench window and "too long = talking" follow YOUR bite length
static void applyClenchTiming() {
  const float d = (float)clenchDurMs;
  float w = d + 1000.0f;
  if (w < (float)DOUBLE_CLENCH_WINDOW_MS) w = (float)DOUBLE_CLENCH_WINDOW_MS;
  if (w > 2600.0f) w = 2600.0f;
  dblClenchMs = (uint32_t)w;
  float l = 1.6f * d + 400.0f;
  if (l < (float)TALK_LONG_BURST_MS) l = (float)TALK_LONG_BURST_MS;
  if (l > 3500.0f) l = 3500.0f;
  talkLongMs = (uint32_t)l;
}

static const char *acStepName() { return (acPhase == AC_BLINK) ? "BLINK" : "CLENCH"; }

static void acBegin() {
  emgSource = EMG_SRC_BOTH;                      // capture with both pills ...
  base = FACTORY_BASE;                           // ... factory rules ...
  thrScale = AC_CAPTURE_SCALE;                   // ... very sensitive: only answers to a beep count
  biteScale = 1.0f;
  applyScales();
  acPhase = AC_REST;
  acPhaseMs = (uint32_t)millis();
  acRestReset = true;                            // the noise is measured afresh once you have settled
  acRestN = 0;
  trainPhase = TRAIN_BLINK;                      // the engine records, never drives
  trainCount = 0;
  trainStepStartMs = acPhaseMs;
  acWinOpen = false;
  preTriggerClear();
  biteLatched = false;
  holdoffSamples = 0;
  enterSettle();
  txPrintf(true, "\n[AUTO] ===== AUTO-CALIBRATION %u/2 (the car will not move) =====\n", (unsigned)acAttempt);
  txPrintf(true, "[AUTO] sit still, eyes open, jaw relaxed ...\n");
}

static void autoCalStart(const char *why) {
  if (sysMode != MODE_RUN) { acBootPending = true; return; }
  if (satV) {
    txPrintf(true, "[AUTO] waiting: the forehead electrodes are not connected (signal clipped)\n");
    acBootPending = true;
    return;
  }
  acBootPending = false;
  if (trainPhase != TRAIN_OFF) trainAbort("calibration restarted");
  motorStopAll();
  seqClear();
  if (lockState != LOCK_LOCKED) lockEngage("calibration");
  acHadSaved = personalized;
  trainSavedBase = base;
  trainSavedThrScale = thrScale;
  trainSavedBiteScale = biteScale;
  trainSavedMinCount = emgMinCount;
  trainSavedStrongCount = emgStrongCount;
  trainSavedSource = emgSource;
  acAttempt = 1;
  txPrintf(true, "[AUTO] calibration started (%s)\n", why);
  acBegin();
}

static void acPrompt() {
  acWinOpen = true;
  acPromptMs = (uint32_t)millis();
  acHaveCand = false;
  acCandScore = 0.0f;
  acWinN = 0;
  ++acPrompts;
  if (acPhase == AC_BLINK) txPrintf(true, "[AUTO] BLINK now - firmly, once (%u/%u)\n", (unsigned)(trainCount + 1), (unsigned)TRAIN_SAMPLES);
  else txPrintf(true, "[AUTO] CLENCH now - bite ~1 s, then relax (%u/%u)\n", (unsigned)(trainCount + 1), (unsigned)TRAIN_SAMPLES);
  buzz(BZ_CLOSE, sizeof(BZ_CLOSE) / sizeof(BZ_CLOSE[0]));
}

// bite length in the window: longest run above 40 % of the window's peak (gaps < 60 ms bridged)
static float acBiteLength() {
  float pk = 0.0f;
  for (uint16_t i = 0; i < acWinN; ++i) if (acWinE[i] > pk) pk = acWinE[i];
  if (pk <= 0.0f) return 0.0f;
  const float lvl = 0.4f * pk;
  uint16_t best = 0, run = 0, gap = 0;
  for (uint16_t i = 0; i < acWinN; ++i) {
    if (acWinE[i] >= lvl) { run = (uint16_t)(run + gap + 1); gap = 0; if (run > best) best = run; }
    else if (run > 0 && ++gap > 15) { run = 0; gap = 0; }
  }
  return (float)best * 1000.0f / FS_F;
}

static void acNextStep();

static void acCloseWindow() {
  acWinOpen = false;
  acNextPromptMs = (uint32_t)millis() + AC_GAP_MS;
  if (acHaveCand) {
    TrainSample &dst = (acPhase == AC_BLINK) ? trainBlink[trainCount] : trainClench[trainCount];
    dst = acCand;
    if (acPhase == AC_CLENCH) acDur[trainCount] = acBiteLength();
    ++trainCount;
    buzz(BZ_REC, sizeof(BZ_REC) / sizeof(BZ_REC[0]));
    txPrintf(true, "[AUTO] %s %u/%u recorded: swing %.0f, EMG %.0f\n", (acPhase == AC_BLINK) ? "blink" : "clench",
             (unsigned)trainCount, (unsigned)TRAIN_SAMPLES, (double)acCand.p2p, (double)acCand.emg);
  } else {
    ++acMisses;
    txPrintf(true, "[AUTO] nothing seen - %s again at the next beep\n", acStepName());
  }
  if (trainCount >= TRAIN_SAMPLES) { acNextStep(); return; }
  if (acPrompts >= AC_MAX_PROMPTS) {
    if (trainCount >= AC_MIN_SAMPLES) {          // enough to work with: repeat the ones we have
      const uint8_t have = trainCount;
      for (uint8_t i = have; i < TRAIN_SAMPLES; ++i) {
        if (acPhase == AC_BLINK) trainBlink[i] = trainBlink[i % have];
        else { trainClench[i] = trainClench[i % have]; acDur[i] = acDur[i % have]; }
      }
      trainCount = TRAIN_SAMPLES;
      acNextStep();
      return;
    }
    txPrintf(true, "[AUTO] could not see your %s (%u of %u) - check the electrodes\n", acStepName(), (unsigned)trainCount, (unsigned)TRAIN_SAMPLES);
    acBlinkOk = acClenchOk = 0;
    acSeparated = false;
    acPhase = AC_SQUEEZE;                        // treat as a failed attempt
    trainPhase = TRAIN_OFF;
    acComplete(false, true);
  }
}

static void acNextStep() {
  if (acPhase == AC_BLINK) {
    acPhase = AC_CLENCH;
    trainPhase = TRAIN_CLENCH;
    trainCount = 0;
    acPrompts = 0;
    acMisses = 0;
    trainStepStartMs = (uint32_t)millis();
    acNextPromptMs = (uint32_t)millis() + 1200;
    txPrintf(true, "[AUTO] blinks done. Now the jaw: CLENCH at each beep.\n");
    return;
  }
  // clenches done: your bite length, then the thresholds, then the unlock squeeze
  float d[TRAIN_SAMPLES];
  for (uint8_t i = 0; i < TRAIN_SAMPLES; ++i) d[i] = acDur[i];
  float med = median5(d);
  if (med < 200.0f) med = 700.0f;
  clenchDurMs = (uint16_t)med;
  applyClenchTiming();
  acPhase = AC_SQUEEZE;
  trainFinish();                                 // thresholds from your samples (not saved yet) -> squeezeStart()
}

// called on every RUN sample
static void acSample(float vEmg, float hEmg) {
  if (acBootPending && trainPhase == TRAIN_OFF && sysMode == MODE_RUN &&
      (uint32_t)((uint32_t)millis() - acReadyMs) >= AC_BOOT_DELAY_MS) {
    autoCalStart("power-on");
    return;
  }
  if (acPhase == AC_OFF || acPhase == AC_SQUEEZE) return;
  const uint32_t now = (uint32_t)millis();
  if (acPhase == AC_REST) {
    if (acRestN >= AC_REST_BUF || (uint32_t)(now - acPhaseMs) >= AC_REST_MAX_MS) {
      acRestApply();
      applyScales();                             // capture thresholds above the measured noise
      acPhase = AC_BLINK;
      trainPhase = TRAIN_BLINK;
      trainCount = 0;
      acPrompts = 0;
      acMisses = 0;
      trainStepStartMs = now;
      acNextPromptMs = now;
      txPrintf(true, "[AUTO] noise: eye %.0f, forehead %.0f, temples %.0f. Now BLINK at each beep.\n",
               (double)nzEog.q, (double)nzEmgV.q, (double)nzEmgH.q);
    }
    return;
  }
  if (acWinOpen) {
    if (acPhase == AC_CLENCH && acWinN < AC_WIN_MAX) acWinE[acWinN++] = fmaxf(vEmg, hEmg);
    const uint32_t win = (acPhase == AC_BLINK) ? AC_BLINK_WIN_MS : AC_CLENCH_WIN_MS;
    const uint32_t open = now - acPromptMs;
    // answered: close early once the blink is over / the jaw has relaxed again
    if (acHaveCand && gState != ST_CAPTURE) {
      // (only for a clear blink: with tremor / noisy electrodes a small wiggle can come
      //  first, and the real blink after it must still get its chance)
      if (acPhase == AC_BLINK && (uint32_t)(now - acCandMs) >= 600UL && acCandScore >= AC_CLEAR_BLINK_K * nzEog.q &&
          acCandScore >= AC_CLEAR_BLINK_MIN) { acCloseWindow(); return; }
      if (acPhase == AC_CLENCH && (uint32_t)(now - acCandMs) >= 400UL) {
        float pk = 0.0f;
        for (uint16_t i = 0; i < acWinN; ++i) if (acWinE[i] > pk) pk = acWinE[i];
        uint16_t quiet = 0;
        for (int i = (int)acWinN - 1; i >= 0 && acWinE[i] < 0.3f * pk; --i) ++quiet;
        if (quiet >= 77) { acCloseWindow(); return; }       // relaxed for 0.3 s
      }
    }
    if (open >= win && (gState != ST_CAPTURE || open >= win + 400)) acCloseWindow();
    return;
  }
  if ((int32_t)(now - acNextPromptMs) >= 0 && (gState == ST_ARMED || (uint32_t)(now - acNextPromptMs) >= 2500UL)) {
    acPrompt();
  }
}

// a finished capture during auto-calibration: keep the strongest one inside the answer window
static void acCapture(float p2p) {
  const uint32_t now = (uint32_t)millis();
  if (acWinOpen && (uint32_t)(now - acPromptMs) >= AC_MIN_REACT_MS) {
    TrainSample smp;
    smp.vmax = cap.winVMax;
    smp.vmin = cap.winVMin;
    smp.p2p = p2p;
    smp.ret = (cap.retMin < 1.0e8f) ? cap.retMin : 0.0f;
    smp.emg = cap.winVEmgMax;
    smp.emgCount = cap.emgAboveCount;
    smp.traceLen = capTraceLen;
    float area = 0.0f;
    for (uint8_t i = 0; i < capTraceLen; ++i) {
      smp.traceV[i] = capTraceV[i];
      smp.traceH[i] = capTraceH[i];
      area += fmaxf(capTraceV[i], capTraceH[i]);
    }
    const float score = (acPhase == AC_BLINK) ? p2p : area;
    if (!acHaveCand || score > acCandScore) {
      acCandMs = now;
      acHaveCand = true;
      acCandScore = score;
      acCand = smp;
    }
  }
  biteLatched = false;                           // same end-of-capture handling as trainRecord()
  if (acPhase == AC_CLENCH) {
    const float rise = chV.level - cap.baseLevel;
    if (rise >= thr.biteLatchMin) latchBite(cap.baseLevel, rise);
  }
  enterSettle();
}

// end of the squeeze step (or a failed step): judge, save or fall back, lock
static void acComplete(bool squeezeOk, bool failedStep) {
  trainPhase = TRAIN_OFF;
  acPhase = AC_OFF;
  acWinOpen = false;
  const bool quality = !failedStep && acSeparated && acBlinkOk >= AC_GOOD_OK && acClenchOk >= AC_GOOD_OK;
  if (!quality && acAttempt < 2) {
    txPrintf(true, "[AUTO] result not clear enough (blinks %u/5, clenches %u/5) - once more\n", (unsigned)acBlinkOk, (unsigned)acClenchOk);
    ++acAttempt;
    buzz(BZ_BROKEN, sizeof(BZ_BROKEN) / sizeof(BZ_BROKEN[0]));
    acBegin();
    return;
  }
  if (!failedStep) {
    unlockBite = !squeezeOk;
    if (unlockBite) {
      txPrintf(true, "[AUTO] your eye squeeze is too weak to see - to UNLOCK, BITE AND HOLD for 3 s instead\n");
    }
  }
  if (quality) {
    trainSave();
    txPrintf(true, "[AUTO] result: calibrated for you (blinks %u/5, clenches %u/5)\n", (unsigned)acBlinkOk, (unsigned)acClenchOk);
  } else if (!failedStep && acBlinkOk >= AC_MIN_OK && acClenchOk >= AC_MIN_OK) {
    trainSave();
    txPrintf(true, "[AUTO] result: partly clear (blinks %u/5, clenches %u/5) - using it\n", (unsigned)acBlinkOk, (unsigned)acClenchOk);
  } else if (acHadSaved && trainLoad()) {
    applyScales();
    txPrintf(true, "[AUTO] result: unclear - using your last good calibration\n");
  } else {
    trainForgetSoft();
    txPrintf(true, "[AUTO] result: unclear - using the factory settings (press NEW / BOOT to try again)\n");
  }
  thrScale = SCALE_DEFAULT;
  biteScale = 1.0f;
  applyScales();
  if (unlockAvgNeed > 0.0f && !unlockBite)
    txPrintf(true, "[AUTO] unlock: hold your eyes closed firmly 3 s (forehead above %.0f, about %.0f on average - a yawn is too weak)\n",
             (double)unlockLevel, (double)unlockAvgNeed);
  txPrintf(true, "[AUTO] done. LOCKED - %s for 3 s to unlock.\n", unlockBite ? "BITE AND HOLD" : "close your eyes firmly");
  lockEngage("calibration finished");
  buzz(BZ_UNLOCK, sizeof(BZ_UNLOCK) / sizeof(BZ_UNLOCK[0]));
}

// ---- buttons (all optional: unconnected pins read HIGH through the pull-up) ----
static void buttonsInit() {
  if (!BUTTONS_ENABLED) return;
  pinMode(PIN_BTN_STOP, INPUT_PULLUP);
  pinMode(PIN_BTN_NEW, INPUT_PULLUP);
  pinMode(PIN_BTN_BOOT, INPUT_PULLUP);
}

static void buttonsService() {
  if (!BUTTONS_ENABLED) return;
  static uint32_t lastMs = 0;
  static uint16_t stopMs = 0, newMs = 0, bootMs = 0;
  const uint32_t now = (uint32_t)millis();
  if ((uint32_t)(now - lastMs) < 10UL) return;
  lastMs = now;
  if (digitalRead(PIN_BTN_STOP) == LOW) {
    if (stopMs < 60000) stopMs += 10;
    if (stopMs == 30) {                          // 30 ms debounce
      motorStopAll();
      seqClear();
      if (acPhase != AC_OFF || trainPhase != TRAIN_OFF) trainAbort("STOP button");
      if (lockState != LOCK_LOCKED) lockEngage("STOP button");
      txPrintf(true, "[BTN] STOP pressed - stopped and LOCKED\n");
    }
  } else {
    stopMs = 0;
  }
  if (digitalRead(PIN_BTN_NEW) == LOW) {
    if (newMs < 60000) newMs += 10;
    if (newMs == 50) autoCalStart("NEW button");
  } else {
    newMs = 0;
  }
  if (digitalRead(PIN_BTN_BOOT) == LOW) {
    if (bootMs < 60000) bootMs += 10;
    if (bootMs == 1000) autoCalStart("BOOT button held 1 s");
  } else {
    bootMs = 0;
  }
}

// factory thresholds without erasing the saved calibration
static void trainForgetSoft() {
  base = FACTORY_BASE;
  emgMinCount = EMG_MIN_COUNT;
  emgStrongCount = EMG_STRONG_COUNT;
  emgSource = EMG_SRC_BOTH;
  unlockLevel = UNLOCK_FACTORY_LEVEL;
  unlockRatio = UNLOCK_FACTORY_RATIO_SET;
  unlockHMax = 0.0f;
  closeEogNeed = 0.0f;
  closeEogOff = false;
  unlockBite = false;
  biteUnlockLevel = 0.0f;
  clenchDurMs = 700;
  applyClenchTiming();
  unlockAvgNeed = 0.0f;
  biteAvgNeed = 0.0f;
  retFree = false;
  personalized = false;
  applyScales();
}

// =============================================================================
// CALIBRATION
// =============================================================================
static void beginCalibration(bool requestedByUser) {
  if (trainPhase != TRAIN_OFF) trainAbort("recalibration");
  motorStopAll();
  sysMode = MODE_CALIBRATE;
  modeCounter = 0;
  calSumH = calSumV = calSqH = calSqV = 0.0;
  preTriggerClear();
  enterSettle();
  biteLatched = false;
  holdoffSamples = 0;
  vOnRail = false;
  satV = satH = false;
  satRunV = clearRunV = satRunH = clearRunH = 0;
  txPrintf(true, "[CAL] %s: sit still, eyes open, jaw relaxed (%u samples = %.1f s)...\n",
           requestedByUser ? "Recalibrating DC offsets" : "Calibrating DC offsets", (unsigned)CAL_SAMPLES,
           (double)CAL_SAMPLES / (double)FS_HZ);
}

static void calibrationStep(float rawH, float rawV) {
  calSumH += (double)rawH;
  calSumV += (double)rawV;
  calSqH += (double)rawH * (double)rawH;
  calSqV += (double)rawV * (double)rawV;
  if (++modeCounter >= CAL_SAMPLES) finishCalibration();
}

static void finishCalibration() {
  const double n = (double)CAL_SAMPLES;
  const double meanH = calSumH / n;
  const double meanV = calSumV / n;
  double varH = calSqH / n - meanH * meanH;
  double varV = calSqV / n - meanV * meanV;
  if (varH < 0.0) varH = 0.0;
  if (varV < 0.0) varV = 0.0;
  const double sdH = sqrt(varH);
  const double sdV = sqrt(varV);

  chH.dcOffset = (float)meanH;
  chV.dcOffset = (float)meanV;
  chH.reset();
  chV.reset();
  levelBase = 0.0f;
  preTriggerClear();
  enterSettle();
  sysMode = MODE_WARMUP;
  modeCounter = 0;

  txPrintf(true, "[CAL] done: V offset %.1f (sd %.1f) | H offset %.1f (sd %.1f) counts\n", meanV, sdV, meanH, sdH);
  const double railTop = 4095.0 - (double)CAL_RAIL_MARGIN;
  if (meanV < (double)CAL_RAIL_MARGIN || meanV > railTop) {
    txPrintf(true, "[WARN] VERTICAL offset near ADC rail: check Pill 2 wiring / electrodes\n");
  }
  if (meanH < (double)CAL_RAIL_MARGIN || meanH > railTop) {
    txPrintf(true, "[WARN] HORIZONTAL offset near ADC rail: check Pill 1 wiring / electrodes\n");
  }
  if (sdV > (double)CAL_NOISY_SD) {
    txPrintf(true, "[WARN] VERTICAL noisy during calibration (sd %.0f): re-seat electrodes, press c\n", sdV);
  }
  txPrintf(true, "[CAL] filter warm-up %.1f s...\n", (double)WARMUP_SAMPLES / (double)FS_HZ);
}

// =============================================================================
// SAMPLE TICK (exactly every 1/256 s)
// =============================================================================
static void emitPlotLine() {
  if (!plotterMode) return;
  if (++plotDecim < PLOT_DECIMATION) return;
  plotDecim = 0;
  char line[112];
  const long trig = lroundf(thr.trigV);
  const int n = snprintf(line, sizeof(line), "H:%ld,V:%ld,Emg:%ld,Motor:%d,TrigHi:%ld,TrigLo:%ld\n",
                         lroundf(chH.eog), lroundf(chV.eog), lroundf(combEmg), (int)motorMarker, trig, -trig);
  if (n > 0 && (size_t)n < sizeof(line)) txEnqueue(line, (size_t)n, false);
}

static void onSampleTick() {
  const float rawH = readAdc(PIN_HORIZ);
  const float rawV = readAdc(PIN_VERT);
  ++sampleIndex;

  switch (sysMode) {
    case MODE_BOOT:
      if (++modeCounter >= BOOT_DELAY_SAMPLES) beginCalibration(false);
      return;
    case MODE_CALIBRATE:
      calibrationStep(rawH, rawV);
      return;
    case MODE_WARMUP:
    case MODE_RUN:
      break;
  }

  chH.process(rawH);
  chV.process(rawV);
  updateSaturation(rawH, rawV);
  // Jaw-muscle EMG from forehead/jaw OR temples (temporalis); a clipped temple channel is ignored.
  if (satH || emgSource == EMG_SRC_FOREHEAD) combEmg = chV.emg;   // clipped temple channel is ignored (v13.2)
  else if (emgSource == EMG_SRC_TEMPLES) combEmg = chH.emg;
  else combEmg = fmaxf(chV.emg, chH.emg);

  if (sysMode == MODE_WARMUP) {
    if (++modeCounter >= WARMUP_SAMPLES) {
      sysMode = MODE_RUN;
      enterSettle();
      txPrintf(true, "[READY] Bite = FORWARD, blink = REVERSE. Arming once the signal is quiet.\n");
      txPrintf(true, "[LOCK] LOCKED - close your eyes firmly for %lu s to unlock (o = unlock by keyboard)\n",
               (unsigned long)(UNLOCK_HOLD_MS / 1000UL));
      buzz(BZ_READY, sizeof(BZ_READY) / sizeof(BZ_READY[0]));
      acReadyMs = (uint32_t)millis();
      if (AUTO_CAL_AT_BOOT) {
        acBootPending = true;                    // FINAL: calibrate by itself
        txPrintf(true, "[AUTO] calibration starts in %.1f s - follow the beeps\n", (double)AC_BOOT_DELAY_MS / 1000.0);
      }
    }
  } else {
    slowRingPush(chV.eog);                       // FINAL: 125 ms history for slow blinks
    gestureStep(chV.eog, combEmg, chV.level);
    seqWatch(chV.eog, combEmg);
    talkSample(combEmg);                         // PRO: jaw burst tracking (talking filter)
    talkService();                               // PRO: deferred TURN RIGHT
    lockSmooth(chV.emg, chH.emg);
    lockEyeMove(chV.eog);                        // also records eyelid movement during training step 3
    lockSample(chV.emg, chH.emg);
    squeezeSample(chV.emg, chH.emg);
    noiseTrack(chV.eog, chV.emg, chH.emg);       // FINAL: resting noise of both channels
    acSample(chV.emg, chH.emg);                  // FINAL: auto-calibration prompts
  }

  preTriggerPush(chV.eog, combEmg, chV.level);
  emitPlotLine();
}

static void advanceTick() {
  uint32_t period = TICK_BASE_US;
  tickFracAcc += TICK_FRAC_NUM;
  if (tickFracAcc >= FS_HZ) {
    tickFracAcc -= FS_HZ;
    ++period;                                    // the 3907 us tick
  }
  nextTickUs += period;
}

// =============================================================================
// SERIAL HOTKEYS
// =============================================================================
static void manualPulse(Gesture g) {
  const char *dir = gestureName(g);
  if (sysMode == MODE_BOOT || sysMode == MODE_CALIBRATE) {
    txPrintf(true, "[CMD] test pulse %s refused: calibration in progress\n", dir);
    return;
  }
  if (motorStart(g)) {
    txPrintf(true, "[CMD] TEST pulse %s (%lu ms of '%c' over ESP-NOW)\n", dir, (unsigned long)motorPulseMs, motorCmd);
  } else {
    txPrintf(true, "[CMD] test pulse %s not sent: motor %s, retry in <1 s (or press s)\n", dir, motorName());
  }
}

static void stopAndRearm() {
  if (trainPhase != TRAIN_OFF) trainAbort("s pressed");
  motorStopAll();
  preTriggerClear();
  biteLatched = false;
  holdoffSamples = 0;
  slowEdgeNoted = false;
  memset(&cap, 0, sizeof(cap));
  if (sysMode == MODE_RUN && !satV) {
    gState = ST_ARMED;
    txPrintf(true, "[CMD] STOP: 'S' sent, buffers cleared, ARMED (live after 39 ms of history)\n");
  } else {
    enterSettle();
    txPrintf(true, "[CMD] STOP: 'S' sent, buffers cleared (%s%s) - arms automatically\n", modeName(sysMode),
             satV ? ", vertical clipped" : "");
  }
}

static void handleHotkey(char c) {
  switch (c) {
    case '1': setThrScale(SCALE_SENSITIVE); printThresholds("Preset 1 SENSITIVE"); break;
    case '2': setThrScale(SCALE_DEFAULT);   printThresholds("Preset 2 DEFAULT"); break;
    case '3': setThrScale(SCALE_STRICT);    printThresholds("Preset 3 STRICT"); break;
    case '+':
    case '=': setThrScale(thrScale + SCALE_STEP); printThresholds("Thresholds +10%"); break;
    case '-':
    case '_': setThrScale(thrScale - SCALE_STEP); printThresholds("Thresholds -10%"); break;
    case '0': setThrScale(SCALE_DEFAULT);   printThresholds("Scale reset"); break;
    case 'j':
    case 'J': setBiteScale(biteScale - BITE_SCALE_STEP); printThresholds("Jaw clench 10% MORE sensitive"); break;
    case 'k':
    case 'K': setBiteScale(biteScale + BITE_SCALE_STEP); printThresholds("Jaw clench 10% LESS sensitive"); break;
    case 'f':
    case 'F': manualPulse(GEST_FORWARD); break;
    case 'b':
    case 'B': manualPulse(GEST_REVERSE); break;
    case 'o':
    case 'O': lockToggleByKey(); break;
    case 'z':
    case 'Z':
      txPrintf(true, "[CMD] buzzer test on GPIO%u (%s)\n", (unsigned)PIN_BUZZER, BUZZER_ENABLED ? "on" : "BUZZER_ENABLED = false");
      buzz(BZ_UNLOCK, sizeof(BZ_UNLOCK) / sizeof(BZ_UNLOCK[0]));
      break;
    case 'l':
    case 'L': manualPulse(GEST_LEFT); break;
    case 'r':
    case 'R': manualPulse(GEST_RIGHT); break;
    case '[': setTurnPulse(turnPulseMs - TURN_PULSE_STEP_MS); break;
    case ']': setTurnPulse(turnPulseMs + TURN_PULSE_STEP_MS); break;
    case 's':
    case 'S': stopAndRearm(); break;
    case 'c':
    case 'C': beginCalibration(true); break;
    case 't':
    case 'T':
    case 'a':
    case 'A': autoCalStart("key"); break;        // FINAL: one calibration for everyone
    case 'y':
    case 'Y': trainStart(); break;               // old keyboard-paced training (advanced)
    case 'u':
    case 'U': trainForget(); break;
    case 'p':
    case 'P':
      plotterMode = !plotterMode;
      plotDecim = 0;
      if (plotterMode) {
        txPrintf(true, "[CMD] Serial Plotter ON (128 Hz: H,V,Emg=max(V,H),Motor,TrigHi,TrigLo) - press p for text\n");
      } else {
        txPrintf(true, "[CMD] Debug text ON - press p for Serial Plotter\n");
      }
      break;
    case 'h':
    case 'H':
    case '?': printHelp(); break;
    case '\r':
    case '\n':
    case ' ':
    case '\t': break;
    default:
      if (c >= 32 && c <= 126) txPrintf(true, "[CMD] unknown key '%c' - press h for help\n", c);
      else txPrintf(true, "[CMD] unknown byte 0x%02X - press h for help\n", (unsigned)(uint8_t)c);
      break;
  }
}

static void serviceSerialInput() {
  for (uint8_t i = 0; i < RX_MAX_PER_LOOP; ++i) {
    if (Serial.available() <= 0) return;
    const int c = Serial.read();
    if (c < 0) return;
    handleHotkey((char)c);
  }
}

// =============================================================================
// HOUSEKEEPING (1 Hz status)
// =============================================================================
static void housekeeping() {
  const uint32_t nowMs = (uint32_t)millis();
  if ((uint32_t)(nowMs - lastStatusMs) < STATUS_PERIOD_MS) return;
  lastStatusMs = nowMs;
  applyScales();                                 // FINAL: refresh the noise floors once per second

  // Long-run sample rate measured between tick start times (phase-exact).
  if (sampleIndex > 0) {
    if (fsRefValid) {
      fsTotalUs += (uint32_t)(lastTickStartUs - fsRefTickUs);
      fsTotalTicks += (uint32_t)(sampleIndex - fsRefIndex);
    }
    fsRefTickUs = lastTickStartUs;
    fsRefIndex = sampleIndex;
    fsRefValid = true;
  }
  const double fsAvg = (fsTotalUs > 0) ? (double)fsTotalTicks * 1.0e6 / (double)fsTotalUs : 0.0;

  if (!plotterMode) {
    switch (sysMode) {
      case MODE_BOOT:
        txPrintf(true, "[BOOT] sit still - calibration starts in %.1f s\n",
                 (double)(BOOT_DELAY_SAMPLES - modeCounter) / (double)FS_HZ);
        break;
      case MODE_CALIBRATE:
        txPrintf(true, "[CAL] %u / %u samples\n", (unsigned)modeCounter, (unsigned)CAL_SAMPLES);
        break;
      case MODE_WARMUP:
        txPrintf(true, "[WARMUP] %.1f s left | V=%+.0f H=%+.0f | EMG V=%.0f H=%.0f (quiet <%.0f)\n",
                 (double)(WARMUP_SAMPLES - modeCounter) / (double)FS_HZ, (double)chV.eog, (double)chH.eog,
                 (double)chV.emg, (double)chH.emg, (double)thr.quietEmg);
        break;
      case MODE_RUN:
        txPrintf(true, "[STAT] %-9s %-7s V=%+5.0f H=%+5.0f EMG V%3.0f H%3.0f | fs=%.3fHz busy=%luus late=%luus ovr=%lu | x%.2f bite x%.2f motor=%s | F%lu R%lu <%lu >%lu ign%lu drop%lu slow%lu | radio '%c' ok%lu fail%lu%s%s%s\n",
                 lockName(), stateName(gState), (double)chV.eog, (double)chH.eog, (double)chV.emg, (double)chH.emg, fsAvg,
                 (unsigned long)statMaxBusyUs, (unsigned long)statMaxLateUs, (unsigned long)overrunCount,
                 (double)thrScale, (double)biteScale, motorName(), (unsigned long)fwdCount, (unsigned long)revCount,
                 (unsigned long)leftCount, (unsigned long)rightCount, (unsigned long)ignoredCount, (unsigned long)busyDropCount, (unsigned long)slowEdgeCount,
                 radioCmd, (unsigned long)radioTxOk, (unsigned long)radioTxFail,
                 radioReady ? "" : " RADIO-OFF", biteLatched ? " BITE" : "", satV ? " CLIPPED" : "");
        break;
    }
    if (txDroppedBytes > 0) {
      txPrintf(true, "[WARN] serial output dropped %lu bytes (host too slow)\n", (unsigned long)txDroppedBytes);
      txDroppedBytes = 0;
    }
  }

  statMaxBusyUs = 0;
  statMaxLateUs = 0;
}

// =============================================================================
// ARDUINO ENTRY POINTS
// =============================================================================
void setup() {
  Serial.begin(SERIAL_BAUD);

  // ADC: core 3.x only accepts per-pin attenuation once the pin is attached
  // to the ADC, so read once first, then set 11 dB, then discard one more read.
  analogReadResolution(ADC_BITS);
  (void)analogRead(PIN_HORIZ);
  (void)analogRead(PIN_VERT);
  analogSetPinAttenuation(PIN_HORIZ, ADC_11db);
  analogSetPinAttenuation(PIN_VERT, ADC_11db);
  (void)analogRead(PIN_HORIZ);
  (void)analogRead(PIN_VERT);

  chH.dcOffset = 2048.0f;
  chV.dcOffset = 2048.0f;
  chH.design((double)FS_HZ);
  chV.design((double)FS_HZ);
  thrScale = SCALE_DEFAULT;
  biteScale = 1.0f;
  const bool trained = trainLoad();
  loadTurnPulse();
  applyScales();
  memset(&cap, 0, sizeof(cap));

  printHelp();
  const esp_reset_reason_t rr = esp_reset_reason();
  txPrintf(true, "[BOOT] reset reason: %s\n", resetReasonName(rr));
  if (rr == ESP_RST_BROWNOUT) {
    txPrintf(true, "[WARN] last reset was a BROWNOUT: the power bank / battery could not hold 3.3 V\n");
  }
  radioInit();                                   // starts broadcasting 'S' immediately
  buzzInit();
  buttonsInit();
  applyClenchTiming();
  printThresholds("Startup");
  if (trained) {
    txPrintf(true, "[TRAIN] personal thresholds loaded from flash (t = train again, u = factory v13.2)\n");
  } else {
    txPrintf(true, "[AUTO] no calibration saved yet: it starts by itself after [READY] - follow the beeps\n");
  }
  txPrintf(true, "[BOOT] Sit still, eyes open, jaw relaxed: DC calibration in %.1f s\n",
           (double)BOOT_DELAY_SAMPLES / (double)FS_HZ);

  sysMode = MODE_BOOT;
  modeCounter = 0;
  preTriggerClear();
  enterSettle();

  const uint32_t nowUs = (uint32_t)micros();
  nextTickUs = nowUs + TICK_BASE_US;
  tickFracAcc = 0;
  lastStatusMs = (uint32_t)millis();
}

void loop() {
  const uint32_t nowUs = (uint32_t)micros();
  const int32_t late = (int32_t)(nowUs - nextTickUs);   // wrap-safe
  if (late >= 0) {
    if ((uint32_t)late > RESYNC_LATE_US) {
      ++overrunCount;                            // stalled > 2 ticks: drop backlog, re-phase
      nextTickUs = nowUs;
      tickFracAcc = 0;
    } else if ((uint32_t)late > statMaxLateUs) {
      statMaxLateUs = (uint32_t)late;
    }
    const uint32_t t0 = (uint32_t)micros();
    lastTickStartUs = t0;
    onSampleTick();
    const uint32_t busy = (uint32_t)micros() - t0;
    if (busy > statMaxBusyUs) statMaxBusyUs = busy;
    advanceTick();
    radioService();                              // transmit right after the ADC reads (see ESP-NOW RADIO)
  }

  serviceSerialInput();
  buttonsService();
  motorService();
  seqService();
  trainService();
  squeezeService();
  lockService();
  buzzService();
  housekeeping();
  txService();
}
