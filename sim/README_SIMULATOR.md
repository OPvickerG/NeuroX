# NeuroX simulator: virtual people that run the real firmware

The simulator compiles the **real firmware** (`NeuroX_Final_Sender.ino`) for a PC, using small Arduino/ESP32 stubs. A signal generator then feeds it electrode signals at 256 Hz. The firmware cannot tell the difference: it prints the same serial log and sends the same radio packets as it would on the box.

## Files

| File | What it does |
|---|---|
| `harness5.cpp` | Signal generator plus fake `analogRead`, `millis`, ESP-NOW and flash. It models blinks, clenches, eye squeezes, talking, chewing, frowns, yawns, head movement, spasms, tremor, nystagmus, mains hum and electrode drift. |
| `Arduino.h`, `Preferences.h`, `WiFi.h`, `esp_*.h` | Minimal stubs, just enough to compile the firmware on a PC. |
| `everyone.py` | Builds the 100 virtual people (11 groups) and runs the tests: auto-calibration, driving, and 4 minutes of everyday life while locked. |
| `few.py` | Runs the same test on a few chosen people, e.g. `python3 few.py ./sim 4 50 85`. |
| `dbg_drive.py` | Shows the firmware log of one person's drive, with what they did and what the car did. |
| `why.py`, `why2.py` | Finds the cause of false unlocks and driving errors. |

## Build and run

```bash
cp ../firmware/NeuroX_Final_Sender/NeuroX_Final_Sender.ino fw_final.cpp
g++ -std=gnu++17 -O2 -w -I. -include Arduino.h harness5.cpp fw_final.cpp -o sim
python3 everyone.py ./sim auto 1 100 result.json      # people 1-100 (≈12 min on 2 cores)
python3 everyone.py ./sim auto 101 100 check.json     # 100 more, never used for tuning
POP=you LIFE=0 python3 everyone.py ./sim auto 1 50    # 50 people built around your own face
```

## How a virtual person works

Each person is built from the creator's measured face (blink size and shape, clench strength, bite length, squeeze, noise). Each value is multiplied by a random factor between 0.7 and 1.4. Then the group's condition is added on top:

| Group | What changes |
|---|---|
| typical | nothing extra |
| weak blink / ptosis / myasthenia | blink 35–60 % as strong, slower, weaker squeeze |
| facial palsy | weak squeeze, smaller blink, weak jaw on one side |
| weak jaw / TMJ / dental | clench 35–55 % as strong, short bites |
| tremor / Parkinson's | 4–6 Hz shaking on both channels, slower blinks |
| spasms / cerebral palsy | sudden muscle spasms, head movements, slower reaction |
| frequent involuntary blinks | 30–60 natural blinks/min plus firm involuntary blinks |
| slow movers / fatigue / elderly | blinks up to 2× slower, long bites, reaction up to 1.5 s |
| noisy electrodes | 1.5–2.5× noise, mains hum, drifting electrodes |
| nystagmus | 2–5 Hz eye oscillation |
| very strong signals | blinks and clenches up to 2× larger |

**Drive test.** This is closed-loop. The person waits for the beeps and the `[READY]` line, just as a real user would. They squeeze to unlock and then make 3 rounds of 6 random gestures (blink, double blink, clench, double clench). Natural blinks and the group's involuntary movements carry on throughout. If the car locks, the person unlocks it again.

**Everyday test.** 4 minutes while LOCKED: blinks, double blinks, glances, looking down/up, talking, chewing, frowning, single clenches, short squints, yawns and head movement. Nothing may unlock it, and the car must not move.

## Virtual you

`me.py` runs the same test on a model of you (your measured signals) in 5 situations: `python3 me.py ./sim exact 50` (also `days`, `tired`, `sweaty`, `rushed`). Results: `docs/VIRTUAL_YOU_TEST.md`.
