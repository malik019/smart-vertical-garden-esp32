# 🌱 Smart Vertical Garden — ESP32-C6 Hydroponics Controller

An automated, modular **Hydroponics Grow Tower** controller powered by an **ESP32-C6 (`ESP32-C6-DevKitC-1`)**. The system autonomously monitors environmental conditions, regulates lighting and irrigation schedules, protects against dry-running pump faults, provides a local asynchronous web interface with OTA updates, and streams real-time telemetry to **MQTT, Telegraf, InfluxDB, and Grafana**.

---

## 🏗️ System Architecture

The project is structured into five decoupled layers:

### 1. Sensors & Measurements (`firmware/` — `sensors.cpp`)
* **Ambient Temperature:** Read every 5 seconds via a **BMP280** sensor over I2C (`GPIO 4` / `GPIO 5`).
* **Reservoir Water Level:** Measured with an **HC-SR04** ultrasonic sensor (`GPIO 6` / `GPIO 7`, protected by a voltage divider on the Echo pin to step 5V down to 3.3V), calibrated between `10 cm = 100%` and `50 cm = 0%`.
* **Water Temperature:** Currently simulated in software and hardware-ready for a 1-Wire **DS18B20** probe (`GPIO 10`).

### 2. Automation & Fail-Safe Protection (`firmware/` — `control.cpp`)
* **NTP Time Synchronization:** Syncs internal time over WiFi via NTP to drive daily schedules accurately.
* **PWM Grow Light Dimming:** Calculates LED brightness (`0–255`) every second via PWM (`GPIO 3`), featuring smooth sunrise and sunset fade windows.
* **Irrigation Cycle & Dry-Run Protection:** Controls the water pump (`GPIO 2`) on a configurable timer. If the reservoir water level drops below the safety threshold, the fail-safe immediately shuts off the pump and triggers a `CRITICAL_LOW_WATER` alarm state.

### 3. Local Web Interface, LittleFS & OTA (`firmware/` — `web_server.cpp`)
* **Non-Hardcoded Web Assets:** Hosted by `ESPAsyncWebServer` with all HTML, CSS, and JavaScript files served directly from internal **LittleFS** flash storage.
* **Live Control & File Management:** View live sensor readings, manually override the pump or LED dimmer, adjust schedules, and perform wireless **Over-The-Air (OTA)** firmware and filesystem updates.
* **JSON Configuration Persistence:** All WiFi credentials and system parameters are stored in `config.json` on LittleFS so settings survive reboots and power loss.

### 4. Power Electronics & PCB Design (`hardware/`)
* **Isolated 12V Switching:** Two **AO3400A Logic-Level MOSFETs** (with `10kΩ` gate pull-down resistors) isolate the 3.3V ESP32-C6 logic from the 12V LED grow lights and 12V water pump.
* **Inductive Kickback Protection:** A **1N914** flyback diode is placed across the pump terminals to suppress voltage spikes.
* **KiCad Hardware Files:** Full schematic, PCB layout, 3D STEP models, and fabrication plots are available in the `hardware/` directory.

### 5. Telemetry & Grafana Monitoring Dashboard
* **Telemetry Pipeline:** `ESP32-C6` → `Mosquitto MQTT (hydro_tower/001/telemetry)` → `Telegraf` → `InfluxDB` → `Grafana`.
* **Real-Time Visualization:**
  * **Gauges:** Water level (`%`), ambient temperature (`°C`), water temperature (`°C`), and LED PWM brightness (`0–255`).
  * **Status Panels:** Live `pump_status` (`ON` / `OFF`) and `alarm` state (`NONE` / `CRITICAL_LOW_WATER`).
  * **Time-Series History:** Historical graphs tracking reservoir water level and temperature trends over time.

---

## 📌 Hardware Pinout (`ESP32-C6-DevKitC-1`)

| Peripheral | Signal / Bus | ESP32-C6 GPIO | Notes |
| :--- | :--- | :--- | :--- |
| **Water Pump (12V)** | MOSFET Gate (PWM/Digital) | `GPIO 2` | AO3400A + 10kΩ pull-down + 1N914 flyback diode |
| **LED Grow Lights (12V)** | MOSFET Gate (PWM) | `GPIO 3` | AO3400A + 10kΩ pull-down, smooth fade window |
| **BMP280 Sensor** | I2C SDA / SCL | `GPIO 4` / `GPIO 5` | 3.3V ambient temperature |
| **HC-SR04 Ultrasonic** | Trigger / Echo | `GPIO 6` / `GPIO 7` | Voltage divider on Echo (`5V -> 3.3V`) |
| **DS18B20 Probe** | 1-Wire Data | `GPIO 10` | Reserved for physical water temperature probe |

---

## 📡 MQTT Telemetry Payload Example

Published periodically to `hydro_tower/001/telemetry`:

```json
{
  "room_temp": 18.78,
  "room_humidity": 52,
  "water_temp": 13.4,
  "water_level": 73,
  "pump_status": "ON",
  "light_brightness": 71,
  "alarm": "NONE"
}
```

---

## 📂 Repository Structure

```text
smart-vertical-garden-esp32/
├── docs/          # Project documentation, diagrams, and dashboard screenshots
├── firmware/      # ESP32-C6 C++ source code (sensors, control, web server) & LittleFS data/
├── hardware/      # KiCad schematics, PCB layout, 3D models, and Gerber/PDF plots
└── README.md      # Project overview and setup guide
```

---

## 🚀 Quick Start

1. **Firmware & LittleFS:** Open `firmware/` in Arduino IDE or PlatformIO (target board: `ESP32C6 Dev Module`), upload the LittleFS filesystem (`data/`), and flash the firmware.
2. **OOTB WiFi Setup:** On first boot without saved credentials, connect to the ESP32-C6 WiFi Manager access point to configure your WiFi network and MQTT settings (saved automatically to `config.json`).
3. **Monitoring Stack:** Configure Mosquitto, Telegraf (`json_string_fields = ["pump_status", "alarm"]`), InfluxDB, and import the Grafana dashboard from `docs/`.
