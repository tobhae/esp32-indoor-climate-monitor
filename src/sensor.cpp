#include "sensor.h"

#include <Arduino.h>
#include <Wire.h>

#include <Adafruit_BME280.h>

#include "config.h"
#include "debug.h"

static Adafruit_BME280 bme;  // BME280 sensor instance (I2C)

ClimateData read_climate() {
  /* Reads temperature, humidity, and pressure from BME280. */
  ClimateData data;
  data.temperature = bme.readTemperature();
  data.humidity = bme.readHumidity();
  data.pressure = bme.readPressure() / 100.0F;

  return data;
}

void init_hardware() {
  /* Initializes serial communication, I2C bus, and BME280 sensor. 
     Halts execution if sensor initialization fails. */
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