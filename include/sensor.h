#pragma once

#include <cstdint>

/* Fixed-point scaling factors */
constexpr uint8_t TEMPERATURE_SCALE = 100;
constexpr uint8_t HUMIDITY_SCALE = 100;
constexpr uint8_t PRESSURE_SCALE = 10;

/* Represents a single climate measurement sample.

   Values are stored as fixed-point integers:
   temperature = °C * TEMPERATURE_SCALE
   humidity = % * HUMIDITY_SCALE
   pressure = hPa * PRESSURE_SCALE */

struct ClimateData {
  int16_t temperature;
  uint16_t humidity;
  uint16_t pressure;
};

ClimateData read_climate();
void init_hardware();