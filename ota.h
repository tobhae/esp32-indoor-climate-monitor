#pragma once 

#include <stdint.h>

/* OTA configuration */

#define OTA_VERSION_URL     ""
#define OTA_FIRMWARE_URL    ""
#define OTA_CHECK_INTERVAL  48  /* Check for update every n cycles */

bool should_check_for_update();
void check_for_update();