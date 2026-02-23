#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <esp_sleep.h>
#include <secrets.h>

#define SEALEVELPRESSURE_HPA (1013.25)
#define uS_TO_S_FACTOR 1000000ULL
#define TIME_TO_SLEEP (15) /* Low value for testing, change to 30 minutes (30 * 60) when deploying. */

/* WiFi credentials (defined in secrets.h) */
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

/* InfluxDB connection parameters (defined in secrets.h) */
const char* influx_host = INFLUX_HOST;
const char* influx_org = INFLUX_ORG;
const char* influx_bucket = INFLUX_BUCKET;
const char* influx_token = INFLUX_TOKEN;

/* Represents a single measurement sample */
struct ClimateData {
  float temperature;  // °C
  float humidity;     // %
  float pressure;     // hPa
};

/* Global runtime objets */
char influx_url[256]; // Buffer for fully constructed InfluxDB write endpoint URL
Adafruit_BME280 bme;  // BME280 sensor instance (I2C)

/*
TODO [Architecture]:
- Remove Serial.print's that might be redudant, and maybe even replace some with a compile-time DEBUG flag.

TODO [Portability]:
- Consider moving location (InfluxDB Line Protocol) to a configurable constant or secrets/config file for easier deployment of multiple nodes.
- Might migrate to ESP8266 due to implications when designing an enclosure for the ESP32 (missing screwholes on the board, cost, etc.).
  * Replace WiFi.h with ESP8266WiFi.h
  * Adjust HTTP client usage
  * Update deep sleep implementation
*/

void setup() {
  init_hardware();
  build_influxdb_url();

  ClimateData data = read_climate();

  if(!connect_wifi()) {
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
  }

  char payload[128];

  if (build_influx_payload(payload, sizeof(payload), data)) {

    if (post_influxdb(payload, strlen(payload))) {
      Serial.println("InfluxDB write successful.");
    } else {
      Serial.println("InfluxDB write failed.");
    }
  }

  /* Shutdown WiFi */
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  /* After successful transmission, enter deep sleep. */
  Serial.println("Transmission finished, going to sleep.");
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
}

void loop() {
  /* Function unused. Execution cycle occurs entirely in setup() and enters deep sleep when finished. */
}

void init_hardware() {
  /* Initializes serial communication, I2C bus, and BME280 sensor. Halts execution if sensor initialization fails. */
  Serial.begin(115200);
  delay(1000);

  /* SDA = GPIO21, SCL = GPIO22 */
  Wire.begin(21, 22);

  if (!bme.begin(0x76) && !bme.begin(0x77)) {
    Serial.println("BME280 not found.");
    while (1);
  }

  Serial.println("BME280 ready.");
}

bool connect_wifi() {
  /* Attempts to connect to configured WiFi network (configured in secrets.h). Returns true if connection succeeds, otherwise false. */
  WiFi.begin(ssid, password);

  unsigned long start = millis();
  const unsigned long timeout = 15000;

  while (WiFi.status() != WL_CONNECTED && millis() - start < timeout) {
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi.");
    return true;
  }

  Serial.println("WiFi connection failed.");
  return false;
}

ClimateData read_climate() {
  /* Reads temperature, humidity, and pressure from BME280. */
  ClimateData data;
  data.temperature = bme.readTemperature();
  data.humidity = bme.readHumidity();
  data.pressure = bme.readPressure() / 100.0F;

  return data;
}

void build_influxdb_url() {
  /* Constructs the full InfluxDB write endpoint URL */
  snprintf(influx_url, sizeof(influx_url), 
           "%s/api/v2/write?org=%s&bucket=%s&precision=s",
           influx_host, influx_org, influx_bucket);
}

bool post_influxdb(const char* payload, size_t len) {
  /* Send provided Line Protocol payload to InfluxDB, returns true if HTTP response is successful (204). */
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection failed.");
    return false;
  }

  HTTPClient http;
  http.begin(influx_url);

  /* Authorization header */
  char auth_header[128];
  snprintf(auth_header, sizeof(auth_header),
          "Token %s", influx_token);

  http.addHeader("Authorization", auth_header);
  http.addHeader("Content-Type", "text/plain; charset=utf-8");

  int httpResponseCode = http.POST((uint8_t*)payload, len);
  http.end();

  return (httpResponseCode == 204);
}

bool build_influx_payload(char* buffer, size_t size, const ClimateData& data) {

  /*
  Formats ClimateData into InfluxDB Line Protocol.

  InfluxDB Line Protocol format:

  measurement, tag=value field1=value1, field2=value2, field3=value3

  NOTE:
  - The measurement name is required by InfluxDB and groups related metrics logically.
  - The tag describes which location the node is deployed in, e.g. kitchen, bathroom, bedroom, or livingroom.
  - The fields corresponds to each metric collected, i.e. temperature, humidity, and pressure.
  */

  int len = snprintf(buffer, size, 
  "indoor_climate,location=test temperature=%.2f,humidity=%.2f,pressure=%.2f",
  data.temperature, data.humidity, data.pressure);

  return (len > 0 && len < size);
}