# NeuroX Final: quick start

## 1. Flash

| Board | Sketch | Arduino IDE settings |
|---|---|---|
| NeuroX box (ESP32-S3) | `NeuroX_Final_Sender/NeuroX_Final_Sender.ino` | **ESP32S3 Dev Module**, *USB CDC On Boot: Enabled* |
| RC car (ESP32 + L298N) | `RC_Car_LR_Receiver/RC_Car_LR_Receiver.ino` (unchanged) | **ESP32 Dev Module** |

- Core **esp32 by Espressif 2.0.17 or 3.x**: both compile.
- Keep each `.ino` in a folder with the same name.
- Serial monitor at **115200 baud** (optional: the box works without a computer).

## 2. Put it on

| Pill | Electrodes |
|---|---|
| GPIO 1 | forehead centre + just above the nose bridge |
| GPIO 4 | left + right temple, at eye level |
| REF | bone behind each ear |

**Battery only** while the electrodes are on. Never use USB or charge with the electrodes on.

## 3. Power on: it calibrates by itself (~40 s)

The car does not move during calibration. Just follow the beeps:

| You hear / see | You do |
|---|---|
| `[READY]` + 2 short beeps, then `sit still, eyes open, jaw relaxed` | Sit still, look ahead (~3 s) |
| one beep + `BLINK now` (× 5) | One firm blink after each beep |
| one beep + `CLENCH now` (× 5) | Bite about 1 s, then relax |
| one beep + `CLOSE your eyes firmly and hold` | Close your eyes firmly (comfortable, not maximum) |
| 2 short beeps + `relax, eyes open` | Open your eyes, relax. Then the 2nd squeeze. |
| long melody + `[AUTO] done. LOCKED` | Calibrated. It is LOCKED. |

**Every power-on recalibrates**, so a new person can simply sit down and switch it on.

- If the result is unclear, it asks once more.
- If it is still unclear, it keeps your last good calibration, or the factory settings if there is none.

## 4. Unlock (3 seconds)

1. **Close your eyes firmly and hold for 3 s.**
   - You hear a tick every second.
   - The monitor shows `[LOCK] holding 1.0 / 3.0 s [##....]`.
2. Melody + `UNLOCKED`: **relax and open your eyes.**
3. Wait for the `READY` beep. Then drive.

Things to know:

- **A yawn does not unlock.** The 3 s must be close to your calibration squeeze in strength.
- If calibration says `your eye squeeze is too weak to see - to UNLOCK, BITE AND HOLD for 3 s instead`, then you unlock by **biting and holding 3 s** (common with facial palsy).
- **3 s without a command → LOCKED again** (one long beep). Every command is a short pulse, so the car is already stopped.

## 5. Drive

| Gesture | Car |
|---|---|
| **Clench** once (bite ~0.5–1 s, relax) | Forward `F` |
| **Blink** firmly once | Reverse `R` |
| **Double blink** | Spin left `<` |
| **Double clench** (bite, relax, bite) | Spin right `>` |

Talking, chewing, looking down at the car and everyday (soft) blinks are filtered out.

## 6. Buttons (optional, each wired to GND)

| Button | Pin | Action |
|---|---|---|
| **STOP** (red) | GPIO 15 | Stop immediately and LOCK |
| **NEW** | GPIO 7 | Calibrate again (new person, or re-gelled electrodes) |
| **BOOT** on the DevKit | GPIO 0 | Hold 1 s = calibrate again |

Buzzer (optional): **+ to GPIO 13, − to GND**.

## 7. Keyboard (optional)

| Key | Action |
|---|---|
| `t` or `a` | Calibrate again |
| `y` | Old keyboard-paced training |
| `u` | Factory thresholds |
| `o` | Lock / unlock |
| `s` | STOP |
| `f` `b` `l` `r` | Test pulses |
| `p` | Plotter ↔ text |
| `h` | Help |

## 8. If something is off

| Problem | Try |
|---|---|
| `nothing seen - BLINK again` during calibration | Blink a bit more firmly; check the forehead pad's gel |
| `unclear - using the factory settings` | Re-gel the pads, sit still, press **NEW** |
| Won't unlock | Close your eyes more firmly for the whole 3 s. Watch `[LOCK] holding`. If it says `too gently`, squeeze like you did in calibration. |
| Turns left when you meant reverse | Blink once, firmly. Don't blink again within about 1 s. |
| Locks while driving | Give the next command within 3 s, or unlock again |
