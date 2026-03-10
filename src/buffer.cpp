#include "buffer.h"

#include <Arduino.h>

#include "debug.h"
#include "influx.h"

/* RTC Buffer */
RTC_DATA_ATTR ClimateSample rtc_buffer[BUFFER_CAPACITY];
RTC_DATA_ATTR uint8_t buffer_head = 0;    // Oldest entry
RTC_DATA_ATTR uint8_t buffer_tail = 0;    // Next write position
RTC_DATA_ATTR uint8_t buffer_count = 0;   // Number of stored entries

/* Store a payload in the RTC buffer, overwriting oldest entry if full */
void buffer_push(const ClimateSample &sample) {
  rtc_buffer[buffer_head] = sample;

  buffer_head = (buffer_head + 1) % BUFFER_CAPACITY;

  if (buffer_count < BUFFER_CAPACITY) {
    buffer_count++;
  } else {
    /* Buffer full, reset to element 0 */
    buffer_tail = (buffer_tail + 1) % BUFFER_CAPACITY;
  }

  /* Debugging */
  DEBUG_PRINT("Buffer push count: ");
  DEBUG_PRINTLN(buffer_count);
  
}

/* Remove the oldest payload from the RTC buffer */
void buffer_pop() {
  if (buffer_count == 0) {
    return;
  }

  buffer_tail = (buffer_tail + 1) % BUFFER_CAPACITY;
  buffer_count--;

  DEBUG_PRINTLN("Buffer entry sent and removed.");
}

/* Attempt to send all buffered payloads in FIFO order */
bool flush_buffer() {
  char payload[PAYLOAD_SIZE];

  while (buffer_count > 0) {
    ClimateSample &sample = rtc_buffer[buffer_tail];

    /* Debugging prints */
    DEBUG_PRINT("Flushing entry, remaining: ");
    DEBUG_PRINTLN(buffer_count);

    if (!build_influx_payload(payload, sizeof(payload), sample.data, sample.timestamp)) {
      return false;
    }

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