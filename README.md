# Distributed Indoor Climate Monitor System

A battery-powered, distributed indoor climate monitoring system built around ESP32 microcontrollers and BME280 environmental sensors. 

Each node periodically measures temperature, humidity, and pressure, transmits data via WiFi to an InfluxDB backend, and enters deep sleep to minimize power consumption. Data is visualized in Grafana for real-time monitoring and long-term analysis.

## System Overview
#### Architecture
ESP32 Nodes → InfluxDB → Grafana Dashboard

Each node operates independently and can be deployed in different locations. Configuration is handled per device without modifying the source code.

#### Tech Stack

- ESP32 firmware (Arduino framework)
- BME280 sensor (I2C)
- InfluxDB 2.7 (Docker)
- Grafana (Docker)
- Li-Ion 18650 battery

## Features
#### Low-Power Operation
The node is designed for long-term battery operation using a single 18650 Li-Ion cell. After completing a measurement and data transmission cycle, the ESP32 enters deep sleep for a configurable interval to minimize power consumption. WiFi is disabled before entering sleep to further reduce current draw during idle periods.

#### Reliable Time-Series Data
Sensor data is transmitted to InfluxDB using the InfluxDB Line Protocol with explicit Unix timestamps. The node synchronizes its system clock with an NTP server to ensure accurate and consistent timestamps across all devices. This guarantees correct time-series ordering in the database, regardless of when the data is ingested. To handle temporary network outages, measurements that cannot be transmitted are stored in an RTC-backed circular buffer and sent during the next successful connection.

#### Configuration Model
The node is configured using a local `config.h` file that defines WiFi credentials, InfluxDB connection parameters, deployment location, and other runtime settings. This approach allows multiple nodes to be deployed
with different configurations without modifying the main firmware source code. Adjustable parameters include the sleep interval, location tag, NTP server, and optional static IP configuration.

#### Deterministic Embedded Design
The firmware is written with a focus on deterministic behavior and predictable memory usage. Payload construction
and networking logic rely on stack-allocated buffers rather than dynamic memory allocation. The codebase is structured to keep hardware interaction, networking, and data transmission logic clearly separated for maintainability.

#### Compile-Time Debug System
Debug output is controlled through centralized macros defined in `debug.h`. Debug statements can be enabled during development and completely removed during compilation for deployment builds. This allows detailed diagnostics when needed without introducing unnecessary serial output in deployed firmware.

#### Over-The-Air Firmware Updates
The firmware supports over-the-air (OTA) updates, allowing deployed nodes to download and install a new firmware version without requiring physical access. The node periodically checks a remote version file hosted in the project 
repository. If a newer firmware version is available, the node downloads the binary and flashes it to the inactive OTA partition. After a successful update the ESP32 reboots, and the bootloader activates the newly installed firmware.

## Project Status
The goal for this project is to create a maintainable and stable embedded system that is suitable for long-term indoor climate monitoring. 
Core functionality is fully operational, including environmental data collection, reliable time-series ingestion, and OTA firmware updates. Development continues with a focus on improving the firmware, documentation, and hardware integration.

An ESP8266-compatible firmware variant is available in a separate branch: [esp8266-node](https://github.com/tobhae/esp-indoor-climate-monitor/tree/esp8266-node)

### Planned Improvements
- 3D-printable enclosure design for the node (_In progress_)
- Rework `debug.h` with configurable log levels (`silent`, `info`, and `debug`)
- Add additional metadata tags to transmitted data (device ID, firmware version, etc.)
- Provide a comprehensive setup and deployment guide in the README
- Maintain and improve ESP8266 compatibility