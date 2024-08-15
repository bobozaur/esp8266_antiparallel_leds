#include <ESP8266WiFi.h>

// Macro for controlling serial debugging
#define DEBUG_SERIAL \
    if (DEBUG)       \
    Serial

#define DEBUG false // set to true for debug output, false for no debug output
#define SSID "MY_SSID"
#define PASSWORD "MY_PASSWORD"
#define TCP_PORT 8080
