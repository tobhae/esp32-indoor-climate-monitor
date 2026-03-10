#include <Arduino.h>

#include "config.h"
#include "debug.h"

#include "network.h"
#include "buffer.h"
#include "influx.h"
#include "sensor.h"
#include "sleep.h"
#include "ota.h"
#include "ntp.h"

/* Firmware entry point and high-level execution flow for the ESP32 node. 
   All work is executed in setup() and the device enter deep sleep when
   the cycle is complete. */

void setup() {
  init_hardware();
  build_influxdb_url();

  if(!connect_wifi()) {
    enter_deep_sleep();
  }

  if(!sync_time()) {
    enter_deep_sleep();
  }

  if(should_check_for_update()) {
    check_for_update();
  }

  ClimateData data = read_climate();
  uint32_t timestamp = time(nullptr);

  char payload[PAYLOAD_SIZE];

  if (!build_influx_payload(payload, sizeof(payload), data, timestamp)) {
    enter_deep_sleep();
  }

  if (!flush_buffer()) {
    ClimateSample sample{data, timestamp};
    buffer_push(sample);
    enter_deep_sleep();
  }

  if (!post_influxdb(payload, strlen(payload))) {
    ClimateSample sample{data, timestamp};
    buffer_push(sample);
  } 

  enter_deep_sleep();

}

void loop() {
  /* Function unused. Execution cycle occurs entirely in setup() and enters deep sleep when finished. */
}