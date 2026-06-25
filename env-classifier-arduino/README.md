# Environmental State Classifier + Fan Control

A decision tree trained on real sensor data, exported to plain C, and run directly on an Arduino UNO R3 -- no ML library on-device, because none fit. The classifier reads ambient light and temperature/humidity, classifies the room into one of four states, and switches a fan on or off accordingly through an NPN transistor.

## Why a decision tree and not a neural network

The UNO R3's ATmega328P has 2KB of RAM, 32KB of flash, no FPU, and no Python interpreter. Every TinyML framework (TensorFlow Lite Micro included) assumes at minimum an ARM Cortex-M with tens to hundreds of KB of RAM -- the UNO doesn't meet that bar by a wide margin.

A trained decision tree, however, is structurally just a sequence of comparisons ending in a label. That compiles down to a handful of `if`/`else` statements and a few float comparisons -- something an 8-bit AVR handles trivially. This project trains the tree offline using real sensor data and scikit-learn, then mechanically translates the tree's internal structure into C code that becomes the actual on-device model. There is no separate "rule-based fallback" -- the generated C *is* the trained classifier.

## Repo structure

```
env-classifier-arduino/
├── README.md
├── images/                              add photos/demo media here
└── src/
    ├── data_collector/
    │   ├── data_collector.ino            Arduino: streams raw sensor readings over serial
    │   └── collect.py                    laptop: captures + labels readings into dataset.csv
    ├── training/
    │   └── train.py                      trains a decision tree, evaluates it, exports tree.h
    ├── deploy/
    │   ├── env_classifier.ino            Arduino: runs the trained model live, drives the fan
    │   └── tree.h                        PLACEHOLDER -- see note below
    └── debug_tools/
        ├── lcd_isolation_test.ino        minimal LCD-only sanity check
        ├── bisect_test_1_sensors.ino      isolation test: LCD + analog sensors only
        ├── bisect_test_2_dht.ino          isolation test: + DHT11
        └── bisect_test_3_full.ino         isolation test: + classify() + fan logic
```

**`deploy/tree.h` is a placeholder, not a real trained model.** It exists so the sketch folder compiles out of the box and so the expected shape of `train.py`'s output is visible without having to run the pipeline first. The actual model only exists after running the full pipeline below on real, freshly-collected sensor data -- it is not something that ships with the repo, since it's specific to one room's lighting and temperature conditions.

## Pipeline

```
1. src/data_collector/data_collector.ino  -> Arduino streams raw sensor readings over serial
2. src/data_collector/collect.py          -> laptop captures + labels readings into dataset.csv
3. src/training/train.py                  -> trains a decision tree, evaluates it, exports tree.h
4. src/deploy/env_classifier.ino + tree.h -> Arduino runs the trained model live, drives the fan,
                                              and displays state + fan status + readings on an LCD
```

## Hardware

| Component | Pin / lead | Connects to |
|---|---|---|
| Thermistor | one leg | 5V |
| Thermistor | other leg | A0, and also to the fixed resistor below (this junction is what A0 reads) |
| Fixed resistor (thermistor divider) | one leg | same junction as thermistor + A0 |
| Fixed resistor (thermistor divider) | other leg | GND |
| Photoresistor | one leg | 5V |
| Photoresistor | other leg | A1, and also to the fixed resistor below (this junction is what A1 reads) |
| Fixed resistor (photoresistor divider) | one leg | same junction as photoresistor + A1 |
| Fixed resistor (photoresistor divider) | other leg | GND |
| DHT11 | signal | D8 |
| DHT11 | VCC / GND | 5V / GND |
| LCD | RS | D7 |
| LCD | RW | GND |
| LCD | E | D6 |
| LCD | D4 | D5 |
| LCD | D5 | D4 |
| LCD | D6 | D3 |
| LCD | D7 | D2 |
| LCD | VSS / VDD | GND / 5V |
| LCD | V0 (contrast) | potentiometer wiper (middle pin) |
| LCD | A (backlight anode) | 5V (through a ~220Ω resistor if your module doesn't already have one built in) |
| LCD | K (backlight cathode) | GND |
| Contrast potentiometer | outer pin 1 | 5V |
| Contrast potentiometer | outer pin 2 | GND |
| Contrast potentiometer | wiper (middle pin) | LCD V0 |
| Base resistor (~1k) | one leg | D9 |
| Base resistor (~1k) | other leg | NPN transistor base |
| NPN transistor | base | base resistor |
| NPN transistor | collector | fan's negative terminal |
| NPN transistor | emitter | shared GND node (see power note below) |
| Flyback diode | across fan terminals | cathode toward fan +, anode toward fan − |
| 9V battery | barrel connector | power supply module's DC input jack |
| Power supply module | voltage selector | set to 5V |
| Power supply module | 5V output rail | fan's positive terminal |
| Power supply module | GND output rail | shared GND node (see power note below) |
| DC fan | positive terminal | power supply module's 5V output |
| DC fan | negative terminal | NPN transistor's collector |

A few notes that aren't obvious from the table alone:

- **Voltage dividers**: the thermistor and photoresistor don't produce a voltage on their own -- each needs a fixed resistor (10kΩ is a reasonable starting value) forming a divider with 5V on one side and GND on the other. The Arduino reads the *junction* between the sensor and the resistor, not either component directly.
- **RW must go to GND**, not be left floating. The `LiquidCrystal` library only ever writes to the display, never reads from it, so RW should be permanently locked into write mode. Leaving it unconnected risks picking up electrical noise and intermittently behaving as if it's in read mode, which causes garbled/corrupted display output.
- **LCD backlight (A/K)** is separate from both the contrast circuit (V0) and the logic pins (RS, E, D4-D7). Without A/K connected, the LCD may still receive commands correctly but the screen will likely be unreadable with no backlight illuminating it.
- **DHT11 is on D8, not D7** -- D7 is claimed by the LCD's RS line in this build, so the DHT11 had to move to avoid a pin conflict.
- **Flyback diode orientation**: it should do nothing during normal fan operation and only conduct briefly when the fan switches off, clamping the voltage spike from the fan's collapsing magnetic field. Cathode (the striped end) faces the 5V/fan+ side.
- **Separate power supply for the fan.** The fan is deliberately powered from a 9V battery through a buck-style power supply module (set to 5V output) rather than the Arduino's own 5V pin. This was a fix for a real, confirmed issue: powering the fan directly off the Arduino's onboard regulator caused a voltage sag at fan startup severe enough to corrupt the LCD's display. The fan's power rail and the Arduino's power rail are kept electrically separate; **only the grounds are tied together**, at one shared node that also includes the transistor's emitter. The two +5V rails are never connected to each other.

The thermistor and DHT11 both measure temperature, deliberately. The thermistor is cheap and fast but uncalibrated (raw ADC counts, not actual degrees); the DHT11 gives a calibrated reading but updates more slowly. Feeding both into the classifier lets the tree decide which one is actually useful for the temp/cool distinction -- worth checking after training which feature(s) the tree actually split on (see `train.py`'s printed tree structure).

## Classes

| Class | Meaning | Fan |
|---|---|---|
| `bright_warm` | bright + warm | ON |
| `bright_cool` | bright + cool | OFF |
| `dark_warm` | dark + warm | ON |
| `dark_cool` | dark + cool | OFF |

Fan logic only depends on the warm/cool half of the classification -- light doesn't affect the actuator decision. This is intentional: it's a good check, after training, that the tree actually learned to split primarily on the temperature-related features when making the fan-relevant distinction, rather than wasting splits on light when light doesn't matter for the action.

## Step-by-step: building this yourself

### 1. Wire the hardware

See the table above. Double-check the flyback diode orientation, and the ground-sharing/power-isolation setup for the fan, before powering anything on.

### 2. Install libraries

In the Arduino IDE: Library Manager -> search "DHT sensor library" (by Adafruit) -> Install. It pulls in a dependency (Adafruit Unified Sensor) automatically. The `LiquidCrystal` library is bundled with the Arduino IDE by default, so no extra install is needed for the LCD.

### 3. Collect training data

Flash `src/data_collector/data_collector.ino`. Then, with the Arduino plugged in and the Serial Monitor *closed* (it locks the port), run:

```bash
pip install pyserial
cd src/data_collector
python collect.py <your_port> bright_warm --seconds 30
python collect.py <your_port> bright_cool --seconds 30
python collect.py <your_port> dark_warm --seconds 30
python collect.py <your_port> dark_cool --seconds 30
```

While each command runs, physically create that condition:
- **bright**: normal room lighting or a lamp pointed at the photoresistor
- **dark**: cup your hand over the photoresistor
- **warm**: breathe warm air near the thermistor, or hold it near (not touching) a warm mug
- **cool**: let it sit normally, or near an open window/fan

Run each condition multiple times across different sessions for variety -- a single 30-second sample per class will likely overfit to that exact moment's lighting and air temperature. Aim for at least 3-4 sessions per class.

This produces `dataset.csv` in `src/data_collector/`, append-only across all your sessions.

### 4. Train the model

```bash
pip install scikit-learn
cd ../training
cp ../data_collector/dataset.csv .
python train.py dataset.csv
```

This prints test accuracy, a classification report, a confusion matrix, and the tree's structure in plain text -- check that it's splitting on sensible features before moving on. It writes `tree.h`.

### 5. Deploy

Copy the generated `tree.h` into `src/deploy/`, replacing the placeholder. Open `src/deploy/env_classifier.ino` in the Arduino IDE (with `tree.h` in the same sketch folder) and upload.

Open Serial Monitor to watch live classifications and fan state as conditions change.

## Design notes

- **Max tree depth is capped at 4** (`train.py --max-depth`). This keeps the generated C code small and avoids overfitting to quirks of a relatively small dataset -- a deeper tree could memorize individual training sessions rather than learning the real light/temperature boundaries.
- **Sensor fusion, not single-sensor thresholds.** The DHT11 and thermistor both relate to temperature; only training reveals which one the model actually relies on. This is a real advantage over hand-written `if` statements -- you don't have to guess which sensor matters most.
- **Stale-reading fallback for the DHT11.** It occasionally returns NaN on a failed read; rather than skip that control cycle entirely (risking a flickering fan), the sketch holds the last valid reading until a new one arrives.
- **LCD display format is deliberately abbreviated.** A 16-character row can't fit the full class name (e.g. `BRIGHT_WARM`, 11 characters) alongside fan status with room to spare, so the LCD shows short codes (`BR-WARM`, `DK-COOL`, etc.) distinct from the full names used over Serial. Row 2 shows raw thermistor, raw photoresistor, and DHT temperature, but leaves out humidity -- worst-case digit counts for all four values together don't fit in 16 characters, and humidity isn't part of the fan's on/off decision in this class scheme, so it was the value to drop from the display (it's still logged in full over Serial).
- **DHT11 reads and LCD writes are on separate timers, and never happen in the same `loop()` pass.** The DHT11's single-wire read protocol briefly disables interrupts while it runs; the `LiquidCrystal` library separately depends on precise timing to pulse its Enable line correctly. Early testing showed Serial output staying clean while the LCD displayed garbled characters, which pointed toward a timing conflict between these two libraries. Separating DHT11 reads onto their own slower timer was a reasonable, real fix for that specific mechanism -- but it turned out not to be the whole story; see **LCD garbling on fan activation** below for what was actually going on.

## Known limitations

- A decision tree with hard thresholds can flip class right at a boundary if a sensor reading sits exactly on it -- there's no hysteresis. A future improvement would add a small dead zone or debounce around the fan's on/off transition to avoid rapid toggling near a boundary.
- The thermistor's voltage-divider raw ADC value is sensitive to the specific resistor used in the divider and isn't portable to a different resistor value without recollecting data.
- **LCD garbling on fan activation (unresolved).** The LCD intermittently displays corrupted characters specifically when the fan's transistor circuit activates -- it stays clean otherwise, including through extended idle operation and DHT11 reads on their own. Sensor readings, classification, and fan actuation are unaffected and were independently verified correct via Serial output throughout.

  This was debugged systematically, not guessed at once and abandoned. In order, the following were each tested and ruled out as the cause:
  - DHT11/LCD interrupt timing conflict (a real, separate, smaller issue -- fixed by moving DHT11 reads onto their own timer, but garbling persisted afterward)
  - Transistor pinout (E-B-C orientation verified against the 2N2222A's actual pin configuration, not just a generic diagram)
  - Flyback diode orientation (verified cathode-toward-power as intended)
  - Transistor failure (swapped for a second, fresh 2N2222A -- identical symptom)
  - Fan lead polarity and routing (verified fan+ goes only to the power supply module's output)
  - Base resistor wiring (verified as an isolated, single-purpose connection from D9 to the base, not shared with anything else)

  A further isolation test found the fault requires the Arduino to be powered and connected -- with the Arduino fully disconnected, the fan/transistor/diode/power-module subcircuit alone never faults, ruling out a passive wiring mistake in that subcircuit in isolation. The fault was also observed to be a standing condition (the power module's own overcurrent protection appears to be latching into a fault state) rather than a one-time transient tied to the instant of connecting a wire.

  The root cause was not conclusively identified within this project's timeframe. Given the substantial list of ruled-out causes above, the most likely remaining explanations are an interaction between the Arduino's GPIO behavior during boot/reset and the transistor's base circuit, or a subtler issue in how the two separate power domains (Arduino's onboard supply, 9V-battery-via-power-module for the fan) interact through their shared ground that wasn't captured by the tests above. Continuing to chase this further would mean systematic voltage measurement with a multimeter or oscilloscope across the relevant nodes during a fault event -- not yet done.

## Debugging tools

`src/debug_tools/` contains a sequence of progressively more complete isolation sketches used to diagnose the LCD garbling issue above, in bisection order:

1. `lcd_isolation_test.ino` -- LCD only, no sensors, no classifier, no fan
2. `bisect_test_1_sensors.ino` -- + thermistor/photoresistor analogRead()
3. `bisect_test_2_dht.ino` -- + DHT11
4. `bisect_test_3_full.ino` -- + classify() and fan digitalWrite (needs a real `tree.h` alongside it to compile)

Useful as a reusable debugging pattern for future projects: when something that touches multiple peripherals misbehaves, strip back to the simplest possible version that's known to work, then add pieces back one at a time until the failure reappears.
