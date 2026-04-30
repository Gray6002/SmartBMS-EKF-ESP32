# Smart BMS with Extended Kalman Filter (EKF)

![Status](https://img.shields.io/badge/Status-Completed-success)
![Platform](https://img.shields.io/badge/Platform-ESP32-blue)
![Framework](https://img.shields.io/badge/Framework-Arduino-orange)

An IoT-enabled Battery Management System (BMS) utilizing an ESP32. This project solves the common "State of Charge (SoC) drift" issue by implementing an Extended Kalman Filter to fuse voltage and current data in real-time.

# Key Features
* **Advanced SoC Estimation:** Replaced basic Coulomb counting with an Extended Kalman Filter (EKF) for dynamic, noise-resilient battery monitoring.
* **Active Thermal Throttling:** Uses an NTC thermistor to monitor pack health. Dynamically derates motor PWM power when temperatures exceed 35°C instead of forcing a hard shutdown.
* **IoT Telemetry & Resilience:** Streams live data via MQTT. Features an offline-fallback using LittleFS to log data locally if Wi-Fi drops, pushing the payload once the connection is restored.
* **Power Optimization:** Implements ESP32 Deep Sleep logic, dropping system current draw during 0mA load periods.

# Hardware Stack
* **Microcontroller:** ESP32 DevKit V1
* **Current/Voltage Sensor:** INA219 (I2C)
* **Display:** 0.96" OLED SSD1306 (I2C)
* **Power Control:** IRFZ44N N-Channel MOSFET driven by a 2N2222 NPN Transistor (Inverted Logic).
* **Temperature:** 10k NTC Thermistor

# Repository Structure
* `/Firmware` - The main C++ source code for the ESP32.
* `/Schematics` - Circuit wiring diagrams.
* `/Media` - Photos of the hardware in action and OLED UI.

# How to Run
1. Install the `Adafruit INA219`, `Adafruit SSD1306`, and `PubSubClient` libraries in Arduino IDE.
2. Select **ESP32 Dev Module** and set Partition Scheme to **Default 4MB with SPIFFS**.
3. Update `ssid` and `password` with your Wi-Fi credentials.
4. Flash to the ESP32 via USB.

---
*Built to demonstrate Embedded C++, Control Systems, and IoT Architecture.*
