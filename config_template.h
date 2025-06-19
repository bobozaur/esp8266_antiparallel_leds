// Needs to be included as we're overridding some macros
#include <ESP8266WiFi.h>

// Macro for controlling serial debugging
#define DEBUG_SERIAL \
    if (DEBUG)       \
    Serial

#define DEBUG false // set to true for debug output, false for no debug output
// WiFi
#define SSID "MY_SSID"
#define PASSWORD "MY_PASSWORD"
// MQTT
#define MQTT_BROKER "MY_BROKER"
#define MQTT_USERNAME "MY_USER"
#define MQTT_PASSWORD "MY_PASSWORD"
#define MQTT_PORT 1883
