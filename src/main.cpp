#include <Arduino.h>

#include "config.h"
#include "debug.h"

#include "buffer.h"
#include "influx.h"
#include "sensor.h"
#include "sleep.h"
#include "wifi.h"
#include "ota.h"
#include "ntp.h"

/* TODO: Some comment or description of the program would be nice. */

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