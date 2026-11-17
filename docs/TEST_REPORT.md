# NeuroX Final: test report, 200 virtual people of every kind

**Firmware:** `NeuroX_Final_Sender.ino` (`v13.2-ESPNOW-r3-LR2-FINAL`), with the **unchanged** `RC_Car_LR_Receiver.ino`
**Setup:** your current hardware. ESP32-S3, 2 × BioAmp EXG Pill (GPIO 1 forehead + nose bridge, GPIO 4 temples, REF behind the ears), ESP-NOW to the RC car.
**Method:** the real firmware source is compiled for a PC and fed simulated electrode signals at 256 Hz (see `simulator/`). Nobody uses a keyboard: every person follows the beeps and serial prompts, like a real user.

> These are simulation results. They show the logic works across very different faces and conditions. Real electrodes on real skin are still the final test (see *What the simulation cannot prove* at the end).

---

## 1. Headline

| | Pro-Safe (before), factory | Pro-Safe (before), keyboard-trained | **Final, people 1–100 (tuned)** | **Final, people 101–200 (never seen)** |
|---|---|---|---|---|
| Calibration | none | needs a keyboard (`t`) | **automatic at power-on** | **automatic at power-on** |
| Driving gestures correct | 75 % | 77 % | **93 %** | **93 %** |
| Wrong car moves while driving | 300 | 228 | **158** | **152** |
| Unlock attempts that failed | 129 / 589 | 149 / 586 | **0 / 330** | **0 / 346** |
| False unlocks in 400 min of everyday life | 135 | 52 | **1** | **0** |
| Car moves while it should be locked | 227 | 84 | **1** | **0** |

Each person drives 18 gestures (3 rounds × 6), with natural blinks and their condition's involuntary movements going on the whole time. Then they spend 4 minutes in everyday life while LOCKED: talking, chewing, yawning, frowning, looking around, blinking, squinting, single clenches and head movements.

**People like you** (50 people built from your own face, each value ±20 %, the same crowd as the earlier Pro-Safe test):

| | Pro-Safe (before) | **Final** |
|---|---|---|
| Driving gestures, clean | 895 / 900 (99.4 %) | **894 / 900 (99.3 %)**: no loss |
| Driving gestures, while also blinking normally | 779 / 900 (86.6 %) | **869 / 900 (96.6 %)** |
| Failed unlocks / false unlocks / locked moves | 0 / 0 / 0 | **0 / 0 / 0** |

---

## 2. Per group (gestures correct · false unlocks · moves while locked)

| Group (8 people each, typical 24, nystagmus & strong 6) | Pro-Safe trained | **Final 1–100** | **Final 101–200** |
|---|---|---|---|
| Typical | 90 % · 16 · 26 | **97 % · 0 · 0** | **98 % · 0 · 0** |
| Weak blink / ptosis / myasthenia | 64 % · 8 · 9 | **91 % · 0 · 0** | **92 % · 0 · 0** |
| Facial palsy (stroke, Bell's) | 87 % · 4 · 14 | **95 % · 0 · 0** | **96 % · 0 · 0** |
| Weak jaw / TMJ / dental | 87 % · 5 · 5 | **96 % · 0 · 0** | **92 % · 0 · 0** |
| Tremor / Parkinson's | 59 % (86 failed unlocks) | **90 % · 0 · 0** | **87 % · 0 · 0** |
| Spasms / cerebral palsy | 77 % · 4 · 3 | **85 % · 0 · 0** | **88 % · 0 · 0** |
| Frequent involuntary blinks | 85 % · 2 · 7 | **94 % · 0 · 0** | **90 % · 0 · 0** |
| Slow movers / fatigue / elderly | 57 % · 7 · 9 | **88 % · 1 · 1** | **88 % · 0 · 0** |
| Noisy electrodes | 48 % · 1 · 0 | **93 % · 0 · 0** | **92 % · 0 · 0** |
| Nystagmus | 72 % (63 failed unlocks) | **95 % · 0 · 0** | **92 % · 0 · 0** |
| Very strong signals | 93 % · 4 · 8 | **94 % · 0 · 0** | **96 % · 0 · 0** |

Auto-calibration result: 199 of 200 people got **"calibrated for you"** and 1 got "partly clear, using it". It takes about **38 s**.

**Remaining errors** (people 101–200, out of 1800 gestures):

| Error | Count | Cause |
|---|---|---|
| Single blink → nothing | 37 | very weak or slow blinks, or a blink during the previous one's settle time |
| Double blink → REVERSE | 26 | 2nd blink too weak or too late |
| Double blink → nothing | 23 | |
| Single blink → TURN LEFT | 18 | an everyday blink right after the command blink |
| Double clench → nothing | 7 | |
| Double clench → REVERSE | 6 | |
| Clench → REVERSE, double blink → FORWARD | 2 + 2 | |

Almost every remaining error is a blink-timing problem. **A blink gesture became forward, or a clench became reverse, only 14 times in 3600 gestures (0.4 %).** The lock never opened by itself in 800 minutes of everyday life.

---

## 3. What the 100-person test found, and what changed

Each problem below was found by the simulation (or in review), traced in the firmware log, fixed, and then re-tested on all 100 people.

| # | Problem found | Who it hit | Fix in the final firmware |
|---|---|---|---|
| 1 | The noise tracker learned from clenches and eye squeezes. The blink trigger then rose above a normal blink, so blinks right after unlocking were invisible. | everyone | Noise is only measured on a calm, armed signal that has been calm for 1 s. |
| 2 | **Yawns unlocked the car.** They were almost all of the 135 false unlocks. | everyone | **Anti-yawn:** the 3 s hold must also average ≥ 35 % of your calibration squeeze. The forehead/temple limit is 2× your clench's. |
| 3 | Noisy electrodes: two bites merged into one long "talking" burst, so double clenches were lost. | noisy | The talking filter's burst level stays above your resting jaw noise. |
| 4 | Electrode drift opened fake captures just before a blink, and drift looked like "looking down". | noisy | The slow-bite trigger and the look-down check grow with the measured drift. |
| 5 | Tremor wiggles were accepted as blinks during calibration. | tremor | A calibration answer must be ≥ 5× the eye noise before its window closes early. |
| 6 | Calibration noise was measured while the person was still squeezing from the step before. | slow, tremor | Calibration waits 1 s, then measures 2 s of calm signal exactly (percentiles). |
| 7 | Slow reactions pushed the squeeze into the "relax" window, and the firmware wrongly switched to bite-to-unlock. | slow / elderly | Resting level comes only from before the first squeeze. Squeeze level uses the 45th percentile. |
| 8 | Slow blinks' dip comes after the 0.2 s window, so they never counted as REVERSE. | slow / elderly | Calibration detects slow blinkers and does not require the dip for them. Looking up is still caught by the look-down check. |
| 9 | Slow double blinks: the window closed in the middle of the 2nd blink. | slow / elderly | The window waits (max 0.6 s) for a 2nd blink that is still under way. |
| 10 | A blink that starts in the previous blink's dip peaks lower and was missed. | everyone | A big rise out of a dip also counts as a blink onset, for both the engine and the double-blink watcher. |
| 11 | An everyday blink just before a command blink became TURN LEFT. | everyone | The two blinks of a double blink must be similar in size. A clearly smaller first blink is ignored. |
| 12 | A clench right after a blink was lost while the blink "settled". | everyone | Sustained jaw muscle after a blink re-arms the engine at once. |
| 13 | A clench's forehead rise was counted as a 2nd blink. | everyone | The double-blink watcher rejects a "blink" that carries jaw muscle. |
| 14 | Long bites split into two bursts, and the next double clench read as "talking". | slow, long bites | A burst ends only after 125 ms of quiet. |
| 15 | Tremor made the same blink measure bigger or smaller, so firm blinks were rejected as "soft". | tremor, nystagmus | The firm-blink limits allow ±3× your eye noise. |
| 16 | The unlock level could rise above your own squeeze (noise measured during a squeeze). | slow | The unlock level is capped at 60 % of your calibration squeeze, so it always stays reachable. |
| 17 | The 3 s relock could cut a gesture that was being measured. | slow | Relock waits for the current capture (0.2 s). |
| 18 | *(found in code review)* For a gentle squeeze (calibration accepts 30–50), the minimum level 30 was applied again after the 60 % cap, so the unlock level ended above 60 % of the squeeze (e.g. 30 for a squeeze of 39 = 77 %). | gentle squeezers | All floors first, then the 60 % cap always wins (e.g. 23 for a squeeze of 39). The cap is still ≥ 1.3 × your relaxed level. Same rule for bite-and-hold (≤ 60 % of your clench) and for the noise-raised level at run time. |

**Test-harness bugs found and fixed:**

- The everyday test replayed the calibration with a different noise seed, which spoiled some people's calibration.
- Weak-squeeze people tried bite-to-unlock even when the firmware hadn't asked them to.

Neither was a firmware bug, but both made the results look worse than they were.

---

## 4. What the simulation cannot prove

- **Real skin and electrode contact.** Gel drying, sweat and pads lifting are only modelled as noise and drift.
- **Real yawns.** Some people close their eyes hard when they yawn. A yawn with a firm eye squeeze for 3 s is physically the same as the unlock gesture.
- **Your exact face.** The model comes from your measured signals, but the first real session will show whether the calibration numbers look like the simulated ones. Compare the `[AUTO] noise`, `[TRAIN] your blink`, `unlock:` lines in the serial monitor.
- **The wheelchair.** All tests drive the RC car. A wheelchair still needs its own receiver, with soft start, a speed cap, the signal-loss stop and a carer e-stop.

**Recommended first real test:** RC car on the floor, serial monitor open.

1. Power on and follow the beeps.
2. Unlock and drive each gesture 10 times.
3. Then sit for 5 minutes talking, eating and yawning, and watch that it stays `LOCKED`.
