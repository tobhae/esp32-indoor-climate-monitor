#include "influx.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include "config.h"
#include "debug.h"
#include "buffer.h"

/* Buffer for fully constructed InfluxDB write endpoint URL */
char influx_url[256]; 

/* Constructs the full InfluxDB write endpoint URL */
void build_influxdb_url() {
  snprintf(influx_url, sizeof(influx_url), 
           "%s/api/v2/write?org=%s&bucket=%s&precision=s",
           INFLUX_HOST, INFLUX_ORG, INFLUX_BUCKET);
}

/* Formats ClimateData into InfluxDB Line Protocol. */
bool build_influx_payload(char* payload, size_t size, const ClimateData& data, uint32_t timestamp) {
  /* InfluxDB Line Protocol format:

     measurement, tag=value field1=value1, field2=value2, field3=value3 timestamp

     NOTE:
     - The measurement name is required by InfluxDB and groups related metrics logically.
     - The tag describes which location the node is deployed in.
     - The fields corresponds to each metric collected, i.e. temperature, humidity, and pressure. */

  /* Conversion from fixed-point integers to float */
  float temperature = data.temperature / (float)TEMPERATURE_SCALE;
  float humidity = data.humidity / (float)HUMIDITY_SCALE;
  float pressure = data.pressure / (float)PRESSURE_SCALE;

  int len = snprintf(payload, size, 
  "climate,location=%s temp=%.2f,hum=%.2f,pres=%.2f %ld",
  NODE_LOCATION, temperature, humidity, pressure, timestamp);

  if (len < 0 || len >= size) {
    DEBUG_PRINTLN("Payload construction failed. ");
    return false;
  }

  return (len > 0 && len < size);
}

/* Send provided Line Protocol payload to InfluxDB, returns true if HTTP response is successful (204). */
bool post_influxdb(const char* payload, size_t len) {
  if (WiFi.status() != WL_CONNECTED) {
    DEBUG_PRINTLN("WiFi connection failed.");
    return false;
  }

  HTTPClient http;
  http.begin(influx_url);

  /* Authorization header */
  char auth_header[128];
  snprintf(auth_header, sizeof(auth_header),
          "Token %s", INFLUX_TOKEN);

  http.addHeader("Authorization", auth_header);
  http.addHeader("Content-Type", "text/plain; charset=utf-8");

  int httpResponseCode = http.POST((uint8_t*)payload, len);
  DEBUG_PRINT("HTTP Response Code: ");
  DEBUG_PRINTLN(httpResponseCode);
  http.end();

  return (httpResponseCode == 204);
}