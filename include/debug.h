#pragma once

/* Provides compile-time debug macros for Serial output.

   DEBUG = 1, debug statements enabled.
   DEBUG = 0, debug statements disabled. 
   
   When disabled, all debug statements are removed during preprocessing. 
   It is recommended to disable DEBUG when deploying a node. */

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