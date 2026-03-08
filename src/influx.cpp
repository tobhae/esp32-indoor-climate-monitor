#include "influx.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include "config.h"
#include "debug.h"

/* TODO: Need to import ClimateData when it has been implemented to separate module. */

/* Buffer for fully constructed InfluxDB write endpoint URL */
char influx_url[256]; 

void build_influxdb_url() {
  /* Constructs the full InfluxDB write endpoint URL */
  snprintf(influx_url, sizeof(influx_url), 
           "%s/api/v2/write?org=%s&bucket=%s&precision=s",
           INFLUX_HOST, INFLUX_ORG, INFLUX_BUCKET);
}

bool build_influx_payload(char* buffer, size_t size, const ClimateData& data) {

  /* Formats ClimateData into InfluxDB Line Protocol.

     InfluxDB Line Protocol format:

     measurement, tag=value field1=value1, field2=value2, field3=value3 timestamp

     NOTE:
     - The measurement name is required by InfluxDB and groups related metrics logically.
     - The tag describes which location the node is deployed in.
     - The fields corresponds to each metric collected, i.e. temperature, humidity, and pressure. */

  time_t now = time(nullptr);

  int len = snprintf(buffer, size, 
  "indoor_climate,location=%s temperature=%.2f,humidity=%.2f,pressure=%.2f %ld",
  NODE_LOCATION, data.temperature, data.humidity, data.pressure, now);

  return (len > 0 && len < size);
}

bool post_influxdb(const char* payload, size_t len) {
  /* Send provided Line Protocol payload to InfluxDB, returns true if HTTP response is successful (204). */
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