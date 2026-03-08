#include "buffer.h"

#include <Arduino.h>

#include "debug.h"
#include "influx.h"

/* RTC Buffer */
RTC_DATA_ATTR char rtc_buffer[BUFFER_CAPACITY][PAYLOAD_SIZE];
RTC_DATA_ATTR uint8_t buffer_head = 0;    // Oldest entry
RTC_DATA_ATTR uint8_t buffer_tail = 0;    // Next write position
RTC_DATA_ATTR uint8_t buffer_count = 0;   // Number of stored entries

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