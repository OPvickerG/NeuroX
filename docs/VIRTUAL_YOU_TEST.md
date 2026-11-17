# Virtual-you test (NeuroX Final)

The real `NeuroX_Final_Sender.ino` was run in the PC simulator on a model of the author, built from his measured
blink, clench, squeeze and noise signals. Each session: power-on auto-calibration, 18 driving commands,
4 minutes of everyday life while locked.

| Situation | Sessions | Commands right | Unlock fails | False unlocks | Moves while locked |
|---|---|---|---|---|---|
| Exact you | 50 | 96.7 % (870/900) | 0 / 155 | 0 | 0 |
| Different days | 30 | 95.6 % | 0 / 96 | 0 | 0 |
| Driving fast | 30 | 95.7 % | 0 / 93 | 0 | 0 |
| Tired | 30 | 94.1 % | 0 / 105 | 0 | 0 |
| Sweaty / old gel | 30 | 93.9 % | 0 / 94 | 0 | 0 |
| **All** | **170** | **95.4 % (2918/3060)** | **0 / 543** | **0 in 680 min** | **0** |

Most errors are single/double blink mix-ups (a natural blink next to a command blink) and missed gestures
when tired or sweaty. Every move is a short pulse (0.42 s, turns 0.3 s), so a wrong one is a nudge.

Run it: `cd sim && python3 me.py ./sim exact 50` (scenarios: exact, days, tired, sweaty, rushed; `SEED0` picks the session set).
