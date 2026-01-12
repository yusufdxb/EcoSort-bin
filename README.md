# EcoSort Bin – Smart Automated Waste-Sorting System

EcoSort is a low-cost embedded systems project that automatically classifies
and sorts waste using multi-sensor fusion and a servo-driven mechanical
platform. The system removes user guesswork by identifying waste items
based on weight, color, reflectivity, and proximity.

> “A Trash Bin with a College Degree”

## Project Motivation
Recycling contamination rates can reach 25–35% in public and institutional
settings due to incorrect disposal. EcoSort addresses this problem by
automating classification using inexpensive sensors and rule-based logic,
without requiring computer vision or machine learning.

## System Overview
EcoSort uses an Arduino-based controller to read data from multiple sensors,
classify waste items, and actuate a seesaw-style platform that directs items
into either trash or recycling compartments.

**Inputs**
- Load cell + HX711 (weight)
- Ultrasonic sensor (presence / height proxy)
- IR reflective sensor (material reflectivity)
- TCS34725 color sensor (RGB)

**Outputs**
- Servo-actuated sorting platform
- OLED display (user feedback)
- Optional buzzer

## Hardware Components
- Arduino Uno R4 WiFi
- Load cell (5 kg) with HX711 amplifier
- HC-SR04 ultrasonic sensor
- TCRT5000 / HW-870 IR reflective sensor
- TCS34725 color sensor
- MG996R servo motor
- SSD1306 OLED display
- Cardboard / prototype enclosure

## Software Features
- Rule-based waste classification
- Multi-sensor fusion on a microcontroller
- Finite state machine for detection, measurement, and actuation
- Real-time OLED feedback
- Modular Arduino firmware

## Results
- 40 classification trials (9 excluded due to mechanical jamming)
- Overall accuracy: **97.5%**
- Correctly classified paper, cardboard, plastic bottles, and mint containers

## Repository Structure
- `src/` – Arduino firmware
- `hardware/` – Wiring diagrams and mechanical design
- `docs/` – Full technical report
- `media/` – Images, diagrams, and demos

## Future Improvements
- Camera-based classification with ML
- IoT logging and analytics dashboard
- Battery-powered operation
- Improved enclosure and mechanics

## How to Run
1. Install Arduino IDE
2. Install required libraries:
   - HX711
   - Adafruit TCS34725
   - Adafruit GFX
   - Adafruit SSD1306
3. Connect hardware according to wiring diagrams
4. Open `src/firmware/ecosort.ino`
5. Upload to Arduino Uno R4
6. Calibrate the load cell before testing

## Bill of Materials (BOM)
| Component | Qty | Cost (USD) |
|---------|-----|------------|
| Arduino Uno R4 WiFi | 1 | $29.00 |
| Load Cell + HX711 | 1 | $6.99 |
| MG996R Servo | 1 | $6.99 |
| OLED Display | 1 | $6.99 |
| TCS34725 Color Sensor | 1 | $3.53 |
| HC-SR04 Ultrasonic | 1 | $3.50 |
| IR Reflective Sensor | 1 | $0.87 |
| Misc. wiring / enclosure | - | ~$10 |
| **Total** |  | **~$70** |


## Author
**Yusuf Guenena**  
M.S. Robotics Engineering – Embedded Systems Design  
Wayne State University
