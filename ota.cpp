#include <WiFi.h>
#include <HTTPClient.h>

#include <ota.h>
#include <firmware_version.h>
#include <debug.h>

RTC_DATA_ATTR uint16_t wake_counter = 0;

/* Decide if OTA should check for update */
bool should_check_for_update() {

    wake_counter++;

    if (wake_counter >= OTA_INTERVAL) {
        wake_counter = 0;
        return true;
    }

    return false;
}

/* Check for update */
void check_for_update() {

    DEBUG_PRINTLN("Checking for OTA update...")
    
}