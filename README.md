# EcoSort Bin

> Embedded waste-sorting prototype using rule-based multi-sensor classification.

EcoSort is an Arduino-based embedded systems project that classifies and sorts waste items using weight, color, reflectivity, and distance sensing. Its strongest signal is practical sensor integration and finite-state machine control on constrained hardware.

## Why This Repo Is Useful

- multi-sensor embedded integration on a low-cost platform
- rule-based classification without hiding behind a vague "AI" label
- actuator control tied to sensing and state transitions
- a complete prototype with BOM and technical report

## System Inputs and Outputs

### Sensors
- load cell + HX711
- TCS34725 color sensor
- IR reflective sensor
- HC-SR04 ultrasonic sensor

### Outputs
- servo-driven sorting platform
- OLED user feedback
- optional buzzer

## Control Approach

The project uses rule-based classification and a finite-state machine, not machine learning. That is a strength here because it matches the actual implementation.

## Results, Framed Honestly

The repo reports:
- 40 classification trials
- 9 trials excluded due to mechanical jamming
- 97.5% classification accuracy on the remaining completed trials

That means the repo currently shows strong classification performance on completed runs, but not yet strong overall mechanical reliability. Both numbers matter, so they should stay separated.

## Repository Layout

| Path | Purpose |
|---|---|
| `src/firmware/ecosort.ino` | Arduino firmware |
| `docs/` | report and supporting documentation |
| `hardware/` | hardware notes |
| `media/` | demo/media placeholders |

## Hardware

| Component | Purpose |
|---|---|
| Arduino Uno R4 WiFi | control board |
| load cell + HX711 | weight sensing |
| TCS34725 | color sensing |
| HC-SR04 | distance / presence |
| IR reflective sensor | material reflectivity cue |
| MG996R servo | sorter actuation |
| SSD1306 OLED | user feedback |

## Results

Supporting docs:

- [docs/RESULTS.md](docs/RESULTS.md) separates classification outcomes from mechanical reliability.
- [docs/templates/trial_log_template.csv](docs/templates/trial_log_template.csv) provides a raw-trial logging format.

## Demo

Current demo link: https://youtu.be/RpR4vYVQu_Y
