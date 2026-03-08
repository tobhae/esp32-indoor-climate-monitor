#pragma once 

#include <stdint.h>

/* Configuration values and function declarations
   for the OTA update module implemented in ota.cpp.  */

#define OTA_VERSION_URL     "https://raw.githubusercontent.com/tobhae/esp-indoor-climate-monitor/ota-implementation/version.txt"
#define OTA_FIRMWARE_URL    ""
#define OTA_CHECK_INTERVAL  48  /* Check for update every n cycles */

bool should_check_for_update();
void check_for_update();