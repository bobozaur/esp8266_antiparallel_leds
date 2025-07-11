#include "config.h"
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define PWM1 4 /* GPIO4 (D2) */
#define PWM2 5 /* GPIO5 (D1) */

WiFiClient espClient;
PubSubClient mqtt_client(espClient);

// State that we reset the PWM state machine to when the flow is over and needs to start again.
char reset_state;

// Current state of the PWM state machine.
volatile char state = 1;

// MQTT Client ID
String clientId = "esp8266-" + String(ESP.getChipId(), HEX);

// Topics
String baseTopic = "esp8266/light/" + clientId;
String configTopic = "homeassistant/light/" + clientId + "/config";
String stateTopic = baseTopic + "/state";
String commandTopic = baseTopic + "/switch";
String brightnessStateTopic = baseTopic + "/brightness";
String brightnessCommandTopic = baseTopic + "/brightness/set";
String availabilityTopic = baseTopic + "/status";

// Discovery Payload
String discoveryPayload = "{\"name\":\"ESP Lights\",\"uniq_id\":\""
                          + clientId + "\",\"stat_t\":\"" + stateTopic + "\",\"cmd_t\":\"" + commandTopic + "\",\"avty_t\":\""
                          + availabilityTopic + "\",\"bri_stat_t\":\"" + brightnessStateTopic + "\",\"bri_cmd_t\":\"" + brightnessCommandTopic
                          + "\",\"bri_scl\":50,\"on_cmd_type\":\"brightness\",\"dev_cla\":\"illuminance\",\"ret\":true,\"schema\":\"basic\",\"dev\":{\"ids\":[\""
                          + clientId + "\"],\"name\":\"ESP Lights\",\"mdl\":\"ESP8266\",\"mf\": \"Bogdan Mircea\"}}";

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

void setup() {
  DEBUG_SERIAL.begin(115200);
  delay(20);

  pinMode(PWM1, OUTPUT);
  pinMode(PWM2, OUTPUT);

  timer1_attachInterrupt(tick);

  connectToWiFi();

  DEBUG_SERIAL.print("MQTT discovery payload: ");
  DEBUG_SERIAL.println(discoveryPayload);
  // Increase buffer size to accommodate the discovery payload.
  mqtt_client.setBufferSize(1024);
  mqtt_client.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt_client.setCallback(mqttCallback);
  connectToMQTTBroker();
}

void connectToWiFi() {
  WiFi.mode(WIFI_STA);

  while (true) {
    WiFi.begin(SSID, PASSWORD);
    DEBUG_SERIAL.println("Connecting to WiFi...");

    if (WiFi.waitForConnectResult() == WL_CONNECTED) {
      DEBUG_SERIAL.print("\nConnected to the WiFi network; IP Address: ");
      DEBUG_SERIAL.println(WiFi.localIP());
      break;
    }

    DEBUG_SERIAL.print("Failed to connect to WiFi, rc=");
    DEBUG_SERIAL.println(WiFi.status());
    delay(1000);
  }
}

void connectToMQTTBroker() {
  while (!mqtt_client.connected()) {
    DEBUG_SERIAL.printf("Connecting to MQTT Broker as %s.....\n", clientId.c_str());

    if (mqtt_client.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD, availabilityTopic.c_str(), 1, true, "offline")) {
      DEBUG_SERIAL.println("Connected to MQTT broker");

      mqtt_client.subscribe(commandTopic.c_str());
      mqtt_client.subscribe(brightnessCommandTopic.c_str());

      mqtt_client.publish(configTopic.c_str(), discoveryPayload.c_str(), true);
      mqtt_client.publish(availabilityTopic.c_str(), "online", true);
    } else {
      DEBUG_SERIAL.print("Failed to connect to MQTT broker, rc=");
      DEBUG_SERIAL.println(mqtt_client.state());
      delay(1000);
    }
  }
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  DEBUG_SERIAL.printf("Handling message from topic: %s\n", topic);

  // Disable timer1 and both PWM1 and PWM2 pins.
  timer1_disable();
  GPOC = (1 << PWM1);
  GPOC = (1 << PWM2);

  if (strcmp(topic, commandTopic.c_str()) == 0 && strncmp((char *)payload, "OFF", length) == 0) {
    mqtt_client.publish(stateTopic.c_str(), "OFF", false);
  } else if (strcmp(topic, brightnessCommandTopic.c_str()) == 0) {
    // Brightness is max 50
    char buffer[3];
    memcpy(buffer, payload, length);
    buffer[length] = '\0';
    unsigned long value = strtoul(buffer, NULL, 10);

    DEBUG_SERIAL.printf("Brightness received: %d\n", value);

    // HAOS does not send a brightness of 0, but check nevertheless.
    if (value) {
      reset_state = 100 / value - 1;
      state = reset_state;
      timer1_enable(TIM_DIV1, TIM_EDGE, TIM_LOOP);
      timer1_write(400);
    }

    mqtt_client.publish(stateTopic.c_str(), "ON", false);
    mqtt_client.publish(brightnessStateTopic.c_str(), buffer, false);
  }
}

void loop() {
  if (!mqtt_client.connected()) {
    connectToMQTTBroker();
  }
  mqtt_client.loop();
}