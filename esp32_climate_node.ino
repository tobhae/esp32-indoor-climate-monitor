#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <esp_sleep.h>
#include <secrets.h>

#define SEALEVELPRESSURE_HPA (1013.25)
#define uS_TO_S_FACTOR 1000000ULL
#define TIME_TO_SLEEP (15) /* Low value for testing*/

// Wifi config
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// InfluxDB config
const char* influx_host = INFLUX_HOST;
const char* influx_org = INFLUX_ORG;
const char* influx_bucket = INFLUX_BUCKET;
const char* influx_token = INFLUX_TOKEN;

char influx_url[256];

Adafruit_BME280 bme;

/*
TODO [Architecture]:
- Refactor WiFi and HTTP logic into dedicated functions to separate networking from sensor logic.
- Move InfluxDB Line Protocol construction into its own function to simplify loop() and improve readability.
- Remove Serial.print's that might be redudant, and maybe even replace some with a compile-time DEBUG flag.

TODO [Power]:
- Change sampling interval from every 10 seconds to every 30 minutes when deploying.

TODO [Portability]:
- Consider moving location (InfluxDB Line Protocol) to a configurable constant or secrets/config file for easier deployment of multiple nodes.
- Might migrate to ESP8266 due to implications when designing an enclosure for the ESP32 (missing screwholes on the board, cost, etc.).
  * Replace WiFi.h with ESP8266WiFi.h
  * Adjust HTTP client usage
  * Update deep sleep implementation
*/

void setup() {
  Serial.begin(115200);
  delay(1000);

  /* SDA = GPIO21, SCL = GPIO22 */
  Wire.begin(21, 22);

  if (!bme.begin(0x76) && !bme.begin(0x77)) {
    Serial.println("BME280 not found.");
    while (1);
  }

  Serial.println("BME280 ready.");

  connect_wifi();

  /* Initialize metrics */
  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F;

  /* Build URL */
  snprintf(
    influx_url, sizeof(influx_url), 
    "%s/api/v2/write?org=%s&bucket=%s&precision=s", 
    influx_host, influx_org, influx_bucket);

  post_influxdb(temperature, humidity, pressure);

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  Serial.println("Transmission finished, going to sleep.");
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void connect_wifi() {
  WiFi.begin(ssid, password);

  unsigned long start = millis();
  const unsigned long timeout = 15000;

  while (WiFi.status() != WL_CONNECTED && millis() - start < timeout) {
    delay(500);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi failed, sleeping anyway.");
    esp_deep_sleep_start();
  } else {
    Serial.println("Connected to WiFi.");
  }
}

void loop() {
  /* Function unused. Execution cycle occurs entirely in setup() and enters deep sleep when finished. */
}

void post_influxdb(float temp, float hum, float pres) {

  if (WiFi.status() != WL_CONNECTED) {
  Serial.println("WiFi not connected!");
  WiFi.reconnect();
  }

  HTTPClient http;
  http.begin(influx_url);

  /* Authorization header */
  char auth_header[128];
  snprintf(auth_header, sizeof(auth_header),
          "Token %s", influx_token);

  http.addHeader("Authorization", auth_header);
  http.addHeader("Content-Type", "text/plain; charset=utf-8");

  /*
  InfluxDB Line Protocol format:

  measurement, tag=value field1=value1, field2=value2, field3=value3

  NOTE:
  - The measurement name is required by InfluxDB and groups related metrics logically.
  - The tag describes which location the node is deployed in, e.g. kitchen, bathroom, bedroom, or livingroom.
  - The fields corresponds to each metric collected, i.e. temperature, humidity, and pressure.
  */

  char payload[128];

  int len = snprintf(
    payload, sizeof(payload), 
    "indoor_climate,location=test temperature=%.2f,humidity=%.2f,pressure=%.2f",
    temp, hum, pres
  );

  if (len < 0 || len >= sizeof(payload)) {
    Serial.println("Payload truncated or error.");
    http.end();
    return;
  }

  int httpResponseCode = http.POST((uint8_t*)payload, len);

  if (httpResponseCode == 204) {
    Serial.println("InfluxDB write successful.");
  } else {
    Serial.print("InfluxDB write failed, code: ");
    Serial.println(httpResponseCode);
  }

  http.end();
}