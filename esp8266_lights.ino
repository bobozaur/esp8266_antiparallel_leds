#include "config.h"
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>

#define PWM1 4 /* GPIO4 (D2) */
#define PWM2 5 /* GPIO5 (D1) */

// State that we reset the PWM state machine to when the flow is over and needs to start again.
char reset_state;
// Current state of the PWM state machine.
volatile char state = 1;

// Timer1 ISR
IRAM_ATTR void tick() {
  switch (state) {
    // Disable PWM1 pin, enable PWM2 pin and reset PWM state machine.
    case 0:
      GPOC = (1 << PWM1);
      GPOS = (1 << PWM2);
      state = reset_state;
      break;

      // Disable PWMw pin, enable PWM1 pin and decrement PWM state machine.
    case 1:
      GPOC = (1 << PWM2);
      GPOS = (1 << PWM1);
      state--;
      break;

      // Disable PWM1 pin, disable PWM2 pin and decrement PWM state machine.
    default:
      GPOC = (1 << PWM1);
      GPOC = (1 << PWM2);
      state--;
      break;
  }
}

// Callback for when an HTTP request is made to the server.
void handleData(void *arg, AsyncClient *client, void *d, size_t len) {
  const char *data = (const char *)d;
  // We only ever expect one byte in the body, so just get the last byte of the buffer.
  // The byte is the PWM duty cycle the client sets.
  // SHOULD NOT BE HIGHER THAN 50!!!!
  char value = data[len - 1];

  DEBUG_SERIAL.printf("Data received from client: %d\n", value);

  // Disable timer1 and both PWM1 and PWM2 pins.
  timer1_disable();
  GPOC = (1 << PWM1);
  GPOC = (1 << PWM2);

  // If a duty cycle higher than 0 is provided, calculate the reset_state and re-enable the timer.
  if (value) {
    reset_state = 100 / value - 1;
    state = reset_state;
    timer1_enable(TIM_DIV1, TIM_EDGE, TIM_LOOP);
    timer1_write(400);
  }
}

// Callback for when a client connects to the server.
static void handleNewClient(void *arg, AsyncClient *client) {
  if (client == NULL) {
    return;
  }

  DEBUG_SERIAL.printf("Client with IP %s has connected to server!\n", client->remoteIP().toString().c_str());

  // Register events
  client->onData(&handleData, NULL);
}

void setup() {
  DEBUG_SERIAL.begin(115200);
  delay(20);

  pinMode(PWM1, OUTPUT);
  pinMode(PWM2, OUTPUT);

  timer1_attachInterrupt(tick);

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  if (WiFi.waitForConnectResult() != WL_CONNECTED) {
    DEBUG_SERIAL.printf("WiFi Failed!\n");
    return;
  }

  DEBUG_SERIAL.print("IP Address: ");
  DEBUG_SERIAL.println(WiFi.localIP());

  AsyncServer *server = new AsyncServer(TCP_PORT);
  server->setNoDelay(true);
  server->onClient(&handleNewClient, server);
  server->begin();
}

void loop() {}