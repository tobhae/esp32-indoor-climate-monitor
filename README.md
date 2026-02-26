# Distributed Indoor Climate Monitor System

A battery-powered, distributed indoor climate monitoring system built around ESP32 microcontrollers and BME280 environmental sensors. 

Each node periodically measures temperature, humidity, and pressure, transmits data via WiFi to an InfluxDB backend, and enters deep sleep to minimize power consumption. Data is visualized in Grafana for real-time monitoring and long-term analysis.

## System Overview
#### Architecture
ESP32 Node(s) → InfluxDB (Docker) → Grafana Dashboard (Docker)

Each node operates independently and can be deployed in different locations. Configuration is handled per device without modifying the source code.

#### Tech Stack

- ESP32 firmware (Arduino framework)
- BME280 sensor (I2C)
- InfluxDB 2.7 (Docker)
- Grafana (Docker)
- Li-Ion 18650 battery

## Features
#### Low-Power Operation
- Deep sleep between transmissions
- WiFi disabled before sleep to reduce power draw
- Designed for 18650 Li-Ion battery operation

#### Reliable Time-Series Data
- NTP synchronization (UTC)
- Explicit Unix timestamps in InfluxDB line protocol
- Correct time-series semantics independent of ingestion time
- RTC-based circular buffer for temporary storage during network outages

#### Configuration model
- Per-node configuration via ```config-h```
- Adjustable sleep interval
- Location tag per node
- Configurable NTP server
- Optional static IP support
- No source code modification required when deploying new nodes

#### Deterministic Embedded Design
- Stack-based payload construction 
- No dynamic ```String``` usage
- Predictable memory footprint
- Clear separation between hardware, networking, and transmission logic

#### Compile-Time Debug System
- Centralized debug macros (```debug.h```)
- Debug statements removed at preprocessing when disabled

## Project Status
The goal for this project is to create a maintanable and stable embedded system that is suitable for long-term indoor climate monitoring. 
Core functionality is currently operational, and further improvements are ongoing. 

An ESP8266-compatible firmware variant is available in a separate branch: [esp8266-node](https://github.com/tobhae/esp-indoor-climate-monitor/tree/esp8266-node)

### Planned Improvements
- Over-The-Air (OTA) update support
- Additional metadata tags (device ID, firmware version, etc.)
- Comprehensive setup guide in the README
- 3D-printable enclosure design
- Continued ESP8266 support