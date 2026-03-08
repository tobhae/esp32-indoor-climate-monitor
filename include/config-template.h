#pragma once

/* This file contains per-node configuration for the climate monitoring system.

   It defines WiFi credentials, InfluxDB connection settings,
   network configuration, and node-specific parameters such as
   sleep interval, physical deployment location, and NTP server.

   Instructions:
   1. Create a new file named "config.h" in the same folder.
   2. Copy the contents of this template into config.h.
   3. Fill in the configuration values.

   Note:
   config.h is ignored by Git and remains local. */

/* WiFi */
#define WIFI_SSID       "<YOUR_WIFI_SSID>"
#define WIFI_PASSWORD   "<YOUR_WIFI_PASSWORD>"

/* InfluxDB */
#define INFLUX_HOST     "http://<INFLUX_SERVER_IP>:8086"
#define INFLUX_ORG      "<INFLUXDB_ORGANIZATION>"
#define INFLUX_BUCKET   "<INFLUXDB_BUCKET>"
#define INFLUX_TOKEN    "<INFLUXDB_TOKEN>"

/* I2C configuration 
   ESP32:   SDA = 21, SCL = 22 */
#define I2C_SDA         21
#define I2C_SCL         22

/* Node configuration */
#define TIME_TO_SLEEP   (30 * 60)         /* Sleep interval in seconds */ 
#define NODE_LOCATION   "<NODE_LOCATION>" /* Physical deployment location (e.g. livingroom, office, etc.) */ 
#define NTP_SERVER      "pool.ntp.org"    /* https://www.ntppool.org/en/ */ 

/* Static IP configuration
   0 = Use DHCP (recommended)
   1 = Use manual static IP settings below */
#define USE_STATIC_IP   0

/* Replace last octet with a free address in your LAN */
#define STATIC_IP_ADDR  192,168,0,100
#define GATEWAY_ADDR    192,168,0,1
#define SUBNET_ADDR     255,255,255,0
#define DNS_ADDR        192,168,0,1