#include "buffer.h"

#include <Arduino.h>
#include <WiFi.h>

#include "debug.h"
#include "influx.h"

/* RTC circular buffer storing ClimateSample entires.

   head: next write position
   tail: oldest entry (next to be sent) 
   count: number of valid samples currently stored */
RTC_DATA_ATTR ClimateSample rtc_buffer[BUFFER_CAPACITY];
RTC_DATA_ATTR uint8_t buffer_head = 0;
RTC_DATA_ATTR uint8_t buffer_tail = 0;
RTC_DATA_ATTR uint8_t buffer_count = 0;

/* Store a ClimateSample in the RTC buffer, overwriting oldest entry if full */
void buffer_push(const ClimateSample &sample) {
  rtc_buffer[buffer_head] = sample;

  /* Advance head to the next write position */
  buffer_head = (buffer_head + 1) % BUFFER_CAPACITY;

  if (buffer_count < BUFFER_CAPACITY) {
    buffer_count++;
  } else {
    /* Buffer full, advance tail to oldest entry */
    buffer_tail = (buffer_tail + 1) % BUFFER_CAPACITY;
  }

  /* Debugging */
  DEBUG_BLOCK({
    Serial.print("Push -> Head: ");
    Serial.print(buffer_head);
    Serial.print(" Tail: ");
    Serial.print(buffer_tail);
    Serial.print(" Count: ");
    Serial.println(buffer_count);
  });
}

/* Remove the oldest sample from the RTC buffer */
void buffer_pop() {
  if (buffer_count == 0) {
    return;
  }

  /* Advance tail to remove oldest entry */
  buffer_tail = (buffer_tail + 1) % BUFFER_CAPACITY;
  buffer_count--;

  /* Debugging prints */
  DEBUG_BLOCK({
    Serial.print("Pop  -> Head: ");
    Serial.print(buffer_head);
    Serial.print(" Tail: ");
    Serial.print(buffer_tail);
    Serial.print(" Count: ");
    Serial.println(buffer_count);
  });
}

/* Attempt to send all buffered samples in FIFO order.
   Each ClimateSample is converted to an InfluxDB payload before sending. */
bool flush_buffer() {
  if (WiFi.status() != WL_CONNECTED) {
    DEBUG_PRINTLN("WiFi not connected, skipping buffer flush.");
    return false;
  }

  char payload[PAYLOAD_SIZE];

  while (buffer_count > 0) {
    ClimateSample &sample = rtc_buffer[buffer_tail];

    /* Debugging prints */
    DEBUG_PRINT("Flushing entry, remaining: ");
    DEBUG_PRINTLN(buffer_count);

    /* Rebuild InfluxDB line protocol payload */
    if (!build_influx_payload(payload, sizeof(payload), sample.data, sample.timestamp)) {
      DEBUG_PRINT("Failed to build payload at buffer index: ");
      DEBUG_PRINTLN(buffer_tail);
      return false;
    }

    /* Send payload to InfluxDB */
    if (!post_influxdb(payload, strlen(payload))) {
      DEBUG_PRINT("Failed to send payload at buffer index: ");
      DEBUG_PRINTLN(buffer_tail);
      return false;
    }

    buffer_pop();
  }

  return true;
}