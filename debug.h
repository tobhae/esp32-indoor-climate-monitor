#pragma once

/* TODO: Some information about debug.h */

#define DEBUG 1

#if DEBUG
    #define DEBUG_PRINT(x)      Serial.print(x)
    #define DEBUG_PRINTLN(x)    Serial.println(x)
    #define DEBUG_BLOCK(x)      do { x; } while (0)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_BLOCK(x)
#endif