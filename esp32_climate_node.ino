#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <esp_sleep.h>
#include <time.h>
#include <config.h>

#define uS_TO_S_FACTOR 1000000ULL

/* WiFi credentials (defined in config.h) */
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;
IPAddress ip(WIFI_STATIC_IP);
IPAddress dns(WIFI_DNS);
IPAddress gateway(WIFI_GATEWAY);
IPAddress subnet(WIFI_SUBNET);

/* InfluxDB connection parameters (defined in config.h) */
const char* influx_host = INFLUX_HOST;
const char* influx_org = INFLUX_ORG;
const char* influx_bucket = INFLUX_BUCKET;
const char* influx_token = INFLUX_TOKEN;

/* Node configuration (defined in config.h) */
const char* time_to_sleep = TIME_TO_SLEEP;

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
- Add a compile-time DEBUG flag for prints, since these prints are unncessary when the node is deployed.

TODO [Portability]:
- Consider moving location (InfluxDB Line Protocol) to a configurable constant or secrets/config file for easier deployment of multiple nodes.
- Might migrate to ESP8266 due to implications when designing an enclosure for the ESP32 (missing screwholes on the board, cost, etc.).
  * Replace WiFi.h with ESP8266WiFi.h
  * Adjust HTTP client usage
  * Update deep sleep implementation

TODO [QoL]:
- Add one retry attempt if post_influxdb fails.
- Failed sends could be stored in a local buffer until connection to InfluxDB/WiFi is restored.
- Consider improving the InfluxDB Line Protocol with new tags, e.g. device_id, etc.
- Over-the-air update. If implemented, this would probably be after deployment and one of the last things to do. 
  * Would require a web server on the network where the nodes can download the new firmware from (using a periodic check).
  * Consider implementing a firmware_version tag (InfluxDB Line Protocol) for easy tracking of not updated nodes.
*/

void setup() {
  init_hardware();
  build_influxdb_url();

  if(!connect_wifi()) {
    esp_sleep_enable_timer_wakeup(time_to_sleep * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
  }

  if(!sync_time()) {
    esp_sleep_enable_timer_wakeup(time_to_sleep * uS_TO_S_FACTOR);
    esp_deep_sleep_start();
  }

  ClimateData data = read_climate();

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
  esp_sleep_enable_timer_wakeup(time_to_sleep * uS_TO_S_FACTOR);
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
  /* Attempts to connect to configured WiFi network (configured in config.h). Returns true if connection succeeds, otherwise false. */
  #if USE_STATIC_IP
  WiFi.config(ip, dns, gateway, subnet);
  #endif

  WiFi.begin(ssid, password);

  unsigned long start = millis();
  const unsigned long timeout = 15000;

  while (WiFi.status() != WL_CONNECTED && millis() - start < timeout) {
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to WiFi.");
    Serial.println(WiFi.localIP());
    return true;
  }

  Serial.println("WiFi connection failed.");
  return false;
}

bool sync_time() {
  /* Synchronizes system time using NTP */
  configTime(0, 0, "europe.pool.ntp.org"); // Consider adding location to a config file

  const unsigned long timeout = 10000;
  unsigned long start = millis();

  time_t now = time(nullptr);

  while(now < 100000 && millis() - start < timeout) {
    delay(200);
    now = time(nullptr);
  }

  if (now < 10000) {
    Serial.println("NTP synchronization failed.");
    return false;
  }

  /* Debugging, print readable time */

  struct tm timeinfo;
  gmtime_r(&now, &timeinfo); // UTC
  char timeString [32];
  strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeinfo);

  Serial.print("NTP Time: ");
  Serial.println(now);

  Serial.print("UTC Time: ");
  Serial.println(timeString);

  Serial.println("Time synchronized.");
  return true;
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

bool build_influx_payload(char* buffer, size_t size, const ClimateData& data) {

  /*
  Formats ClimateData into InfluxDB Line Protocol.

  InfluxDB Line Protocol format:

  measurement, tag=value field1=value1, field2=value2, field3=value3 timestamp

  NOTE:
  - The measurement name is required by InfluxDB and groups related metrics logically.
  - The tag describes which location the node is deployed in, e.g. kitchen, bathroom, bedroom, or livingroom.
  - The fields corresponds to each metric collected, i.e. temperature, humidity, and pressure.
  */

  time_t now = time(nullptr);

  int len = snprintf(buffer, size, 
  "indoor_climate,location=test temperature=%.2f,humidity=%.2f,pressure=%.2f %ld",
  data.temperature, data.humidity, data.pressure, now);

  return (len > 0 && len < size);
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