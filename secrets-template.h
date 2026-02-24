#pragma once

/* This project requires a local secrets.h file containing WiFi and InfluxDB credentials.

   You need to:
   1. Create a new file named "secrets.h" in the same folder.
   2. Copy the contents of this template into secrets.h.
   3. Fill in your own credentials.

   Note:
   The file "secrets.h" is ignored by Git and remains local. */

/* WiFi */
#define WIFI_SSID       "<YOUR_WIFI_SSID>"
#define WIFI_PASSWORD   "<YOUR_WIFI_PASSWORD>"

/* InfluxDB */
#define INFLUX_HOST     "http://<INFLUX_SERVER_IP>:8086"
#define INFLUX_ORG      "<INFLUXDB_ORGANIZATION>"
#define INFLUX_BUCKET   "<INFLUXDB_BUCKET>"
#define INFLUX_TOKEN    "<INFLUXDB_TOKEN>"

/* Static IP configuration
   0 = Use DHCP
   1 = Use manual static IP settings below */
#define USE_STATIC_IP   0

/* Replace last octet with a free address in your LAN */
#define WIFI_STATIC_IP  192,168,0,100
#define WIFI_DNS        192,168,0,1
#define WIFI_GATEWAY    192,168,0,1
#define WIFI_SUBNET     255,255,255,0

