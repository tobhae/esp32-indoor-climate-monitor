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

// TODO: Review code

void setup() {
  Serial.begin(115200);
  delay(1000);

  // SDA = GPIO21, SCL = GPIO22
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

  Serial.print("WiFi status: ");
  Serial.println(WiFi.status()); // 3 = connected, 6 = disconnected

  send_to_influx(temperature, humidity, pressure);

  delay(10000); // Send every 10 seconds

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

  // InfluxDB line protocol, edit first line when deploying
  String data = "environment,location=livingroom ";
  data += "temperature=" + String(temp);
  data += ",humidity=" + String(hum);
  data += ",pressure=" + String(pres);

  int httpResonseCode = http.POST(data);

  Serial.print("HTTP Response: ");
  Serial.println(httpResonseCode);

  http.end();
}