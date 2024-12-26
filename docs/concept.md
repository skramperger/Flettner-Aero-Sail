# Project Concept: Flettner-Rotor-Powered Boat

## Overview
This project develops a model boat powered by a Flettner rotor, utilizing the Magnus effect for propulsion. It demonstrates an innovative and sustainable method for moving small vessels.

## Goals
- Build a functional model boat with Flettner rotor propulsion.
- Develop a custom controller for intuitive remote operation.
- Integrate sensors for real-time environmental data collection.

## Key Features
- **Flettner Rotor Propulsion:** Efficient use of wind energy.
- **ESP32 Communication:** Real-time wireless control between boat and controller.
- **Custom Controller:** Ergonomic design with joysticks, buttons, and an OLED display.
- **Environmental Monitoring:** Data collection on wind, temperature, and other variables.

## Architecture
- **Boat ESP32:** Handles sensor data and propulsion control.
- **Controller ESP32:** Processes user inputs and displays telemetry.
- **Communication:** Wi-Fi for data exchange between the two ESP32 units.

## Technology Stack
- **Microcontrollers:** ESP32 WROOM 32
- **Display:** OLED for telemetry
- **Sensors:** Wind speed, temperature, humidity
- **Fabrication:** 3D-printed boat hull and controller case

## Challenges Addressed
- Creating an efficient rotor-driven propulsion system.
- Ensuring seamless communication between devices.
- Providing real-time feedback to the user.

