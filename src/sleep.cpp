#include "sleep.h"

#include <Arduino.h>
#include <esp_sleep.h>
#include <WiFi.h>

#include "config.h"
#include "debug.h"

void enter_deep_sleep() {
  DEBUG_PRINTLN("Shutting down WiFi and entering deep sleep.");

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * 1000000ULL);
  esp_deep_sleep_start();
}