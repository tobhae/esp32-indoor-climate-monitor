#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <secrets.h>

#define SEALEVELPRESSURE_HPA (1013.25)

// Wifi config
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// InfluxDB config
const char* host = INFLUX_HOST;
const char* org = INFLUX_ORG;
const char* bucket = INFLUX_BUCKET;
const char* token = INFLUX_TOKEN;

Adafruit_BME280 bme;

/*
TODO [Architecture]:
- Refactor WiFi and HTTP logic into dedicated functions to separate networking from sensor logic.
- Move InfluxDB Line Protocol construction into its own function to simplify loop() and improve readability.
- Remove Serial.print's that might be redudant, and maybe even replace some with a compile-time DEBUG flag.

TODO [Performance]:
- Replace blocking delay() with non-blocking timing (millis()) if continious runtime mode is required.
- Optimize HTTP request handling and reduce repeated object creation
  * Pre-build the static URL once.
  * Use a stack buffer instead of String when building the payload.

TODO [Power]:
- Implement deep sleep between sampling (just using delay() for testing right now) to increase battery-life when deployed.
- Evaluate disabling WiFi immediately after successful transmission.
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

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED); {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.println(WiFi.localIP());

  Serial.println("Starting BME280...");

  if (!bme.begin(0x76) && !bme.begin(0x77)) {
    Serial.println("BME280 not found.");
    while (1);
  }

  Serial.println("BME280 ready.");
}

void loop() {

  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();
  float pressure = bme.readPressure() / 100.0F;

  Serial.println("Sending data to InfluxDB.");

  /*
  WiFi debugging
  3 = connected
  6 = disconnected
  */
  Serial.print("WiFi status: ");
  Serial.println(WiFi.status());

  send_to_influx(temperature, humidity, pressure);

  
  /* Send data every 10 sec. */
  delay(10000);

}

void send_to_influx(float temp, float hum, float pres) {

  if (WiFi.status() != WL_CONNECTED) {
  Serial.println("WiFi not connected!");
  WiFi.reconnect();
  }

  HTTPClient http;

  String url = String(INFLUX_HOST) + "/api/v2/write?org=" + INFLUX_ORG + "&bucket=" + INFLUX_BUCKET + "&precision=s";

  http.begin(url);

  http.addHeader("Authorization", "Token " + String(INFLUX_TOKEN));
  http.addHeader("Content-Type", "text/plain; charset=utf-8");

  /*
  InfluxDB Line Protocol format:

  measurement, tag=value field1=value1, field2=value2, field3=value3

  NOTE:
  - The measurement name is required by InfluxDB and groups related metrics logically.
  - The tag describes which location the node is deployed in, e.g. kitchen, bathroom, bedroom, or livingroom.
  - The fields corresponds to each metric collected, i.e. temperature, humidity, and pressure.
  */

  String data = "environment,location=livingroom ";
  data += "temperature=" + String(temp);
  data += ",humidity=" + String(hum);
  data += ",pressure=" + String(pres);

  int httpResonseCode = http.POST(data);

  Serial.print("HTTP Response: ");
  Serial.println(httpResonseCode);

  http.end();
}