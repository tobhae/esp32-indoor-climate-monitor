#pragma once

/*
This project requires a local secrets.h file containing WiFi and InfluxDB credentials.

You need to:
1. Create a new file named "secrets.h" in the same folder.
2. Copy the contents of this template into secrets.h.
3. Fill in your own credentials.

Note:
The file "secrets.h" is ignored by Git and remains local.
*/

/* WiFi */
#define WIFI_SSID "your_ssid"
#define WIFI_PASSWORD "your_wifi_password"

/* InfluxDB */
#define INFLUX_HOST "your_ipv4_address"
#define INFLUX_ORG "influxdb_organization"
#define INFLUX_BUCKET "influxdb_bucket"
#define INFLUX_TOKEN "influxdb_token"