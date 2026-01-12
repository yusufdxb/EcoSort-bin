## Firmware

This folder contains the Arduino firmware responsible for:
- Reading all sensor inputs
- Performing waste classification
- Driving the servo-based sorting mechanism
- Displaying system feedback on the OLED

The firmware is implemented as a finite state machine to ensure
stable operation and repeatable measurements.
