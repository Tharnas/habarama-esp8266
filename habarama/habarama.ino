#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library

#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#include <PubSubClient.h>         // Client to connect to cloud via mqtt
#include <ArduinoJson.h>          // Library to parse Json. Used for payload of mqtt
#include <stdio.h>


#include "settings.h"


// configuration for sensor
#define sensor_name "......"
#define sensor_type "......"
#define sensor_location "......"


WiFiClientSecure espClient;
PubSubClient client(espClient);

// configuration for actor.
actor_type actors[] = {
  // {D0, 0, "actor.<location>.<actor name 1>"},
  // {D1, 0, "actor.<location>.<actor name 2>"}
};

long lastSensorDataSent = 0;
long lastReconnect = 0;

void setup() {
  Serial.begin(115200);

  if(!loadSettings()){
    Serial.println("Failed to load settings, save them");
    if(!saveSettings()){
      Serial.println("Oh, noooo");
    }
  }

  initializeWifi();

  initializeActors();

  initializeMqtt();
}


void initializeWifi() {
  WiFiManager wifiManager;
  // wifiManager.resetSettings(); // This can be used to rest wifi settings.
  wifiManager.autoConnect();
}

void initializeActors() {
  for (byte i = 0; i < (sizeof(actors) / sizeof(*actors)); i++) {
    actors[i].ticks_until = 0;
    pinMode(actors[i].pin, OUTPUT);
    digitalWrite(actors[i].pin, LOW);
  }
}

void initializeMqtt() {
  espClient.allowSelfSignedCerts();
  client.setServer(Settings.mqtt_host, Settings.mqtt_port);
  client.setCallback(mqtt_callback);
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("mqtt topic: ");
  Serial.println(topic);

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(payload);

  for (byte i = 0; i < (sizeof(actors) / sizeof(*actors)); i++) {

    // Check if topic matches to actor
    if (strcmp(topic, actors[i].topic.c_str()) == 0) {
      Serial.println("actor found");

      if (!root.success()) {
        Serial.println("Could not parse payload from mqtt");
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
    }
  }
}

void setGpio(actor_type *actor, long duration) {
  Serial.print("Setting actor to hight: ");
  Serial.println(actor->topic);

  actor->ticks_until = millis() + duration;

  digitalWrite(actor->pin, HIGH);
}

void reconnect() {
  // reconnect when not connected and 5s passed since last try.
  if (!client.connected() && (lastReconnect == 0 || (millis() - lastReconnect) > 5000)) {
    Serial.print("Attempting MQTT connection...");

    // Attempt to connect
    if (client.connect("ESP8266Client", Settings.mqtt_user, Settings.mqtt_pwd)) {
      Serial.println("connected");

      subscribeToMqtt();
    } else {
      Serial.print("failed, rc=");
      Serial.println(client.state());
    }
  }
}

void subscribeToMqtt() {

  // subscribe all actors to mqtt
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

}

void loop() {

  // set pin of actor to low when ticks_until passed.
  for (byte i = 0; i < (sizeof(actors) / sizeof(*actors)); i++) {
    if (actors[i].ticks_until < millis() && digitalRead(actors[i].pin)) {
      Serial.print("Setting actor to low: ");
      Serial.println(actors[i].topic);
      digitalWrite(actors[i].pin, LOW);
    }
  }

  if (!client.connected()) {
    reconnect();
  }

  if (client.connected()) {
    client.loop();

    // send each 60s sensor data to cloud.
    long now = millis();
    if (now - lastSensorDataSent > 60000) {
      lastSensorDataSent = now;

      sendSensorData();
    }
  }
}

void sendSensorData() {
  String payload;

  // read sensor data
  int val = analogRead(0);

  /* Create json payload.
     Example:
     {
       "sensorName": "<name of sensor>",
       "type": "<type of sensor>",
       "value": <value read from sensor (0 - 1023)>,
       "location": "<location of sensor>",
       "version": <version of this sensor>
      }
  */
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
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
