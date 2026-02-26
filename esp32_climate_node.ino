#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <esp_sleep.h>
#include <time.h>
#include <config.h>
#include <debug.h>

/* Represents a single measurement sample */
struct ClimateData {
  float temperature;  // °C
  float humidity;     // %
  float pressure;     // hPa
};

/* Global runtime objets */
char influx_url[256]; // Buffer for fully constructed InfluxDB write endpoint URL
Adafruit_BME280 bme;  // BME280 sensor instance (I2C)

/* RTC Buffer */
#define BUFFER_CAPACITY 10
#define PAYLOAD_SIZE 128
RTC_DATA_ATTR char rtc_buffer[BUFFER_CAPACITY][PAYLOAD_SIZE];
RTC_DATA_ATTR uint8_t buffer_head = 0;    // Oldest entry
RTC_DATA_ATTR uint8_t buffer_tail = 0;    // Next write position
RTC_DATA_ATTR uint8_t buffer_count = 0;   // Number of stored entries

/* TODO [QoL]:
   - Consider improving the InfluxDB Line Protocol with new tags, e.g. device_id, etc.
   - Over-the-air update. If implemented, this would probably be after deployment and one of the last things to do. 
     * Would require a web server on the network where the nodes can download the new firmware from (using a periodic check).
     * Consider implementing a firmware_version tag (InfluxDB Line Protocol) for easy tracking of not updated nodes. */

void setup() {
  init_hardware();
  build_influxdb_url();

  if(!connect_wifi()) {
    enter_deep_sleep();
  }

  if(!sync_time()) {
    enter_deep_sleep();
  }

  ClimateData data = read_climate();

  char payload[PAYLOAD_SIZE];

  if (!build_influx_payload(payload, sizeof(payload), data)) {
    enter_deep_sleep();
  }

  if (!flush_buffer()) {
    buffer_push(payload);
    enter_deep_sleep();
  }

  if (!post_influxdb(payload, strlen(payload))) {
    buffer_push(payload);
  } 

  enter_deep_sleep();

}

void loop() {
  /* Function unused. Execution cycle occurs entirely in setup() and enters deep sleep when finished. */
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
    while (1);
  }

  DEBUG_PRINTLN("BME280 ready.");
}

bool connect_wifi() {
  /* Attempts to connect to configured WiFi network */
  #if USE_STATIC_IP
  WiFi.config(
    IPAddress (WIFI_STATIC_IP),
    IPAddress (WIFI_DNS),
    IPAddress (WIFI_GATEWAY),
    IPAddress (WIFI_SUBNET));
  #endif

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();
  const unsigned long timeout = 15000;

  while (WiFi.status() != WL_CONNECTED && millis() - start < timeout) {
    delay(500);
  }

  if (WiFi.status() == WL_CONNECTED) {
    DEBUG_PRINT("Connected to WiFi with IP: ");
    DEBUG_PRINTLN(WiFi.localIP());
    return true;
  }

  DEBUG_PRINTLN("WiFi connection failed.");
  return false;
}

bool sync_time() {
  /* Synchronizes system time using NTP */
  configTime(0, 0, NTP_SERVER);

  const unsigned long timeout = 10000;
  unsigned long start = millis();

  time_t now = time(nullptr);

  while(now < 100000 && millis() - start < timeout) {
    delay(200);
    now = time(nullptr);
  }

  if (now < 10000) {
    DEBUG_PRINTLN("NTP synchronization failed.");
    return false;
  }

  DEBUG_BLOCK({
    /* Debugging prints */
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo); // UTC
    char timeString [32];
    strftime(timeString, sizeof(timeString), "%Y-%m-%d %H:%M:%S", &timeinfo);

    Serial.print("NTP: ");
    Serial.println(now);

    Serial.print("UTC: ");
    Serial.println(timeString);

    Serial.println("Time synchronized.");
  });

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

void enter_deep_sleep() {
  DEBUG_PRINTLN("Shutting down WiFi and entering deep sleep.");

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * 1000000ULL);
  esp_deep_sleep_start();
}

bool buffer_push(const char* payload) {
  /* Store a payload in the RTC buffer, overwriting oldest entry if full */
  if (buffer_count >= BUFFER_CAPACITY) {
    /* Buffer full, overwrite oldest entry */
    buffer_head = (buffer_head + 1) % BUFFER_CAPACITY;
    buffer_count--;
  }

  strncpy(rtc_buffer[buffer_tail], payload, PAYLOAD_SIZE - 1);
  rtc_buffer[buffer_tail][PAYLOAD_SIZE - 1] = '\0';

  buffer_tail = (buffer_tail  + 1) %  BUFFER_CAPACITY;
  buffer_count++;

  /* Debugging */
  DEBUG_PRINT("Buffer push count: ");
  DEBUG_PRINTLN(buffer_count);
  
  return true;
}

const char* buffer_peek() {
  /* Return pointer to oldest buffered payload without removing it (nullptr if empty) */
  if (buffer_count == 0) {
    return nullptr;
  }

  return rtc_buffer[buffer_head];
}

void buffer_pop() {
  /* Remove the oldest payload from the RTC buffer */
  if (buffer_count == 0) {
    return;
  }

  buffer_head = (buffer_head + 1) % BUFFER_CAPACITY;
  buffer_count--;

  DEBUG_PRINTLN("Buffer entry sent and removed.");
}

bool flush_buffer() {
  /* Attempt to send all buffered payloads in FIFO order */
  while (buffer_count > 0) {
    const char* payload = buffer_peek();

    /* Debugging prints */
    DEBUG_PRINT("Flushing entry, remaining: ");
    DEBUG_PRINTLN(buffer_count);

    if (!post_influxdb(payload, strlen(payload))) {
      return false;
    }

    buffer_pop();

    /* Debugging prints */
    DEBUG_BLOCK({
      Serial.print("Head: ");
      Serial.println(buffer_head);
      Serial.print("Tail: ");
      Serial.println(buffer_tail);
    });
  }

  return true;
}