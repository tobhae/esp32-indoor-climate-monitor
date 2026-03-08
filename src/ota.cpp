#include "ota.h"

#include <string.h>

#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>

#include "firmware_version.h"
#include "debug.h"

/* Provides Over-The-Air (OTA) firmware update functionality
   for the indoor climate monitor node.
   
   The module periodically checks a remote version file to
   determine whether a newer firmware version is available.
   If a newer version is detected, the firmware binary is 
   downlaoded from a GitHub and written to the inactive
   partition. */

RTC_DATA_ATTR uint16_t wake_counter = 0;

/* Update the firmware to the latest version */
bool perform_update() {
/* Firmware is written to the inactive OTA partition
   and the running firmware is never overwritten.

   After Update.end() succeeds, the ESP32 bootloader
   switches the active partition on the next reboot. */

    DEBUG_PRINTLN("Starting OTA update...");

    HTTPClient http;
    http.begin(OTA_FIRMWARE_URL);
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    int httpCode = http.GET();

    DEBUG_PRINT("HTTP code: ");
    DEBUG_PRINTLN(httpCode);

    if (httpCode != HTTP_CODE_OK) {
        DEBUG_PRINTLN("Firmware download failed.");
        http.end();
        return false;
    }

    int content_length = http.getSize();

    if (content_length <= 0) {
        DEBUG_PRINTLN("Invalid firmware size.");
        http.end();
        return false;
    }

    DEBUG_PRINT("Firmware size: ");
    DEBUG_PRINTLN(content_length);

    WiFiClient* stream = http.getStreamPtr();

    if (!Update.begin(content_length)) {
        DEBUG_PRINTLN("OTA begin failed.");
        http.end();
        return false;
    }

    size_t written = Update.writeStream(*stream);

    if (written != content_length) {
        DEBUG_PRINTLN("OTA write incomplete.");
        Update.abort();
        http.end();
        return false;
    }

    if (!Update.end()) {
        DEBUG_PRINTLN("OTA end failed.");
        http.end();
        return false;
    }

    if (!Update.isFinished()) {
        DEBUG_PRINTLN("OTA not finished.");
        http.end();
        return false;
    }

    DEBUG_PRINTLN("OTA update successful.");
    DEBUG_PRINTLN("Rebooting...");

    http.end();

    delay(1000);
    ESP.restart();

    return true;
}

/* Decide if OTA should check for update */
bool should_check_for_update() {

    wake_counter++;

    if (wake_counter >= OTA_CHECK_INTERVAL) {
        wake_counter = 0;
        return true;
    }

    return false;
}

/* Check for update */
void check_for_update() {

    DEBUG_PRINTLN("Checking for OTA update...");

    HTTPClient http;
    http.begin(OTA_VERSION_URL);

    int httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK) {
        DEBUG_PRINTLN("OTA version request failed.");
        http.end();
        return;
    }

    WiFiClient* stream = http.getStreamPtr();

    char remote_version[32];
    size_t index = 0;

    while (stream->available() && index < sizeof(remote_version) - 1) {
        char c = stream->read();

        if (c == '\n' || c == '\r') {
            break;
        }

        remote_version[index++] = c;
    }

    remote_version[index] = '\0';

    http.end();

    DEBUG_PRINT("Remote firmware version: ");
    DEBUG_PRINTLN(remote_version);

    if (strcmp(remote_version, FIRMWARE_VERSION) != 0) {
        DEBUG_PRINTLN("New firmware available.");
        perform_update();
    } else {
        DEBUG_PRINTLN("Firmware is up to date.");
    }   
}