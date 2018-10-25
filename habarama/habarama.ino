#include <ESP8266WiFi.h>

#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic


#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <stdio.h>


#define sensor_name "......"
#define sensor_type "......"
#define sensor_location "......"

#define mqtt_server "broker-amq-mqtt-ssl-57-hogarama.cloud.itandtel.at"
#define mqtt_server_port 443
#define mqtt_user "mq_habarama"
#define mqtt_password "mq_habarama_pass"

WiFiClientSecure espClient;
PubSubClient client(espClient);

typedef struct {
  byte pin;
  long ticks_until;
  String topic;
} actor_type;

actor_type actors[] = {
  // {D0, 0, "actor.<location>.<actor name 1>"},
  // {D1, 0, "actor.<location>.<actor name 2>"}
};

void setup() {
  Serial.begin(115200);

  WiFiManager wifiManager;
  wifiManager.autoConnect();

  for (byte i = 0; i < (sizeof(actors) / sizeof(*actors)); i++) {
    pinMode(actors[i].pin, OUTPUT);
    digitalWrite(actors[i].pin, LOW);
  }

  espClient.allowSelfSignedCerts();
  client.setServer(mqtt_server, mqtt_server_port);
  client.setCallback(mqtt_callback);
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("mqtt topic: ");
  Serial.println(topic);

  
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(payload);

  for (byte i = 0; i < (sizeof(actors) / sizeof(*actors)); i++) {
    if (strcmp(topic, actors[i].topic.c_str()) == 0) {
      Serial.println("actor found");

      if (!root.success()) {
        Serial.println("ParsenObject() failed");
        return;
      }

      const char* type = root["type"];
      long duration = root["duration"];

      if (strcmp(type, "gpio") == 0) {
        setGpio(&actors[i], duration);
      } else {
        Serial.println("Unknown actor type");
        return;
      }

      break;
    }
  }
}

void setGpio(actor_type *actor, long duration) {
  actor->ticks_until = millis() + duration;

  digitalWrite(actor->pin, HIGH);
  Serial.println("Actor set");
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");

    // Attempt to connect
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      Serial.println("connected");

      for (byte i = 0; i < (sizeof(actors) / sizeof(*actors)); i++) {
        if (actors[i].topic.length()) {
          if (client.subscribe(actors[i].topic.c_str())) {
            Serial.println("Successfully subscribed");
          }
          else {
            Serial.println("Failed to subscribe....");
          }
        }
      }
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

long lastMsg = 0;

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  for (byte i = 0; i < (sizeof(actors) / sizeof(*actors)); i++) {
    if (actors[i].ticks_until < millis()) {
      digitalWrite(actors[i].pin, LOW);
    }
  }

  long now = millis();
  if (now - lastMsg > 60000) {
    lastMsg = now;

    int val = analogRead(0);

    String payload;
    StaticJsonBuffer<200> jsonBuffer;

    JsonObject& root = jsonBuffer.createObject();
    //  '{{"sensorName": "name", "type": "type", "value": value, "location": "location", "version": 1 }}'
    root["sensorName"] = sensor_name;
    root["type"] = sensor_type;
    root["value"] = val;
    root["location"] = sensor_location;
    root["version"] = 1;

    root.printTo(payload);

    Serial.print("Sending: ");
    Serial.println(payload);
    char payloadChar[payload.length() + 1];

    payload.toCharArray(payloadChar, payload.length() + 1);

    client.publish("habarama", payloadChar, true);
  }
}
