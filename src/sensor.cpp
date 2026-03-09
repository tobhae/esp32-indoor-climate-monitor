#include "sensor.h"

#include <Arduino.h>
#include <Wire.h>

#include <Adafruit_BME280.h>

#include "config.h"
#include "debug.h"

/* BME280 sensor instance (I2C) */
static Adafruit_BME280 bme;

/* Reads temperature, humidity, and pressure from BME280. */
ClimateData read_climate() {
  ClimateData data;
  data.temperature = (int16_t)(bme.readTemperature() * TEMPERATURE_SCALE);
  data.humidity = (uint16_t)(bme.readHumidity() * HUMIDITY_SCALE);
  data.pressure = (uint16_t)((bme.readPressure() / 100.0f) * PRESSURE_SCALE);

  return data;
}

/* Initializes serial communication, I2C bus, and BME280 sensor. 
   Restarts ESP32 if sensor initialization fails. */
void init_hardware() {
  DEBUG_BLOCK({
    Serial.begin(115200);
    delay(500);
  });

  Wire.begin(I2C_SDA, I2C_SCL);

  if (!bme.begin(0x76) && !bme.begin(0x77)) {
    DEBUG_PRINTLN("BME280 not found.");
    DEBUG_PRINTLN("Rebooting...");
    delay(500);
    ESP.restart();
  }

  DEBUG_PRINTLN("BME280 ready.");
}