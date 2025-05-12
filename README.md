<img src="images/logo.jpg" alt="Flettner Aero Sail Logo" width="250">

# Flettner-Aero-Sail  
*Remote-controlled catamaran powered by a Flettner rotor (HTL diploma project 2025)*

---

## What this repo contains
| Folder | Content |
|--------|---------|
| `/firmware/controller` | Arduino-ESP32 source for the hand-held controller |
| `/firmware/boat`       | Arduino-ESP32 source for the on-board MCU |
| `/hardware/CAD`        | Fusion 360 models (`.f3d`) and printable STLs of hull, rotor & enclosure |
| `/hardware/wiring`     | PDF schematics and pin-out drawings |
| `/docs`                | Excerpts of the thesis, test logs, pool-trial photos |

---

## Project at a glance
|  |  |
|--|--|
| **Propulsion** | 3-D-printed Flettner rotor (Ø 61 mm × 263 mm) driven by a 2200 KV brushless DC motor via HTD-3 mm timing belt |
| **Hull** | 500 mm × 300 mm catamaran → high stability & shallow draft |
| **Control link** | **ESP-NOW** (direct peer-to-peer, < 2 ms latency, optional AES-128) |
| **Hand-held TX** | 2-axis joystick (rudder) • rotary encoder w/ push (rotor RPM) • 128×32 px OLED |
| **On-board RX** | ESC 40 A bidirectional • high-torque servo DS3218 for rudder |
| **Materials** | PETG HF (hulls), PLA Matte (rotor), ASA CF (rotor mount), TPU ( gaskets ) – all printed on a Bambu Lab P1S |
| **Field tests** | Validated in a self-built 2 × 1 m water tank; detailed logs and video in `/docs/tests` |

---

## Key features
- **Magnus-effect drive** &nbsp;— explore an alternative, low-noise propulsion concept  
- **Full open hardware** &nbsp;— CAD files, STL, wiring, bill of materials included  
- **Low-latency radio** &nbsp;— robust control up to ~ 70 m without any router  
- **Inline failsafe** &nbsp;— programmable watchdog → neutral rudder & motor stop if link lost  
- **Config via `static constexpr`** &nbsp;— all pin maps & time-outs in one header, easy to port  
- **Modular build** &nbsp;— hull halves, rotor and controller can be re-printed or replaced separately  

---

## Quick start

### 1 · Clone
```bash
git clone https://github.com/skramperger/Flettner-Aero-Sail.git
