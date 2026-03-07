#include <WiFi.h>
#include <HTTPClient.h>
#include <string.h>

#include "firmware_version.h"
#include "debug.h"
#include "ota.h"

RTC_DATA_ATTR uint16_t wake_counter = 0;

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

    /* HTTP_CODE_OK = 200,
       HTTP_CODE_CREATED = 201,
       HTTP_CODE_ACCEPTED = 202,
       HTTP_CODE_NO_CONTENT = 204,
       HTTP_CODE_BAD_REQUEST = 400,
       HTTP_CODE_UNAUTHORIZED = 401,
       HTTP_CODE_FORBIDDEN = 403,
       HTTP_CODE_NOT_FOUND = 404 */

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

    if (strncmp(remote_version, FIRMWARE_VERSION, sizeof(remote_version)) != 0) {
        DEBUG_PRINTLN("New firmware available.");
        // perform_update();
    } else {
        DEBUG_PRINTLN("Firmware is up to date.");
    }   
}