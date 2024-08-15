# esp8266_antiparallel_leds
Anti parallel LED chain driver using an ESP8266 and an H bridge.

Configuration can be achieved by renaming the `config_template.h` to `config.h` and adjusting it to your environment.

The server only reads the last byte from the request (can be used with either a plain TCP connection that sends one byte or an HTTP request where the body contains a single byte) and uses that as the duty cycle.

The duty cycle should not be larger than 50, since this code is meant for an antiparallel chain of LEDs so each half of the LEDs can run at maximum 50% duty cycle.
