#include <ESP8266WiFi.h> //ESP8266 Core WiFi Library

#include <DNSServer.h>        //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h> //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>      //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <WiFiUdp.h>          //Udp library that is required for ntp client
#include <NTPClient.h>        //NTP client to get date and time for ssl connection.

#include <PubSubClient.h> // Client to connect to cloud via mqtt
#include <ArduinoJson.h>  // Library to parse Json. Used for payload of mqtt
#include <stdio.h>

#include "settings.h"

/* ntp */
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

/* mqtt */
WiFiClientSecure secureEspClient;
WiFiClient espClient;
PubSubClient client;

long lastSensorDataSent = 0;
long lastReconnect = 0;

void setup()
{
  Serial.begin(115200);

  if (!loadSettings())
  {
    Serial.println("Failed to load settings, save them");
    if (!saveSettings())
    {
      Serial.println("Saving empty settings was also not possible. Did you select any \"Flash size: 4M (xxM SPIFFS)\"");
    }
  }

  initializeWifi();

  initializeWebServer();

  initializeActors();

  initializeMqtt();

  timeClient.begin();

  Serial.println("setup done");
}

void initializeWifi()
{
  WiFiManager wifiManager;
  // wifiManager.resetSettings(); // This can be used to rest wifi settings.
  wifiManager.autoConnect();
}

void initializeActors()
{
  for (byte i = 0; i < (sizeof(Settings.actors) / sizeof(*Settings.actors)); i++)
  {
    if (Settings.actors[i].topic[0] != '\0')
    {
      Serial.print("Initializing actor: ");
      Serial.println(Settings.actors[i].pin);
      Settings.actors[i].ticks_until = 0;
      pinMode(Settings.actors[i].pin, OUTPUT);
      digitalWrite(Settings.actors[i].pin, LOW);
    }
  }
}

void initializeMqtt()
{
  client.setCallback(mqtt_callback);
}

void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("mqtt topic: ");
  Serial.println(topic);

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject &root = jsonBuffer.parseObject(payload);

  for (byte i = 0; i < (sizeof(Settings.actors) / sizeof(*Settings.actors)); i++)
  {
    // Check if topic matches to actor
    if (strcmp(topic, Settings.actors[i].topic) == 0)
    {
      Serial.println("actor found");

      if (!root.success())
      {
        Serial.println("Could not parse payload from mqtt");
        return;
      }

      const char *type = root["type"];
      long duration = root["duration"];

      if (strcmp(type, "gpio") == 0)
      {
        setGpio(&Settings.actors[i], duration);
      }
      else
      {
        Serial.println("Unknown actor type");
        return;
      }
    }
  }
}

void setGpio(actor_type *actor, long duration)
{
  Serial.print("Setting actor to high: ");
  Serial.println(actor->topic);

  actor->ticks_until = millis() + duration;

  digitalWrite(actor->pin, HIGH);
}

void reconnect()
{
  // reconnect when not connected and 10s passed since last try.
  if (!client.connected() && (lastReconnect == 0 || (millis() - lastReconnect) > 10000) && Settings.mqtt_host[0] != '\0')
  {
    Serial.print("Connecting to mqtt: ");
    Serial.print(Settings.mqtt_host);
    Serial.print(":");
    Serial.println(Settings.mqtt_port);

    if (Settings.mqtt_ssl)
    {
      if (!timeClient.update())
      {
        timeClient.forceUpdate();
      }

      if (!timeClient.update())
      {
        Serial.println("Getting time failed.");
        // updating of ntp was not successful, break here.
        return;
      }

      Serial.print("Current time: ");
      Serial.println(timeClient.getFormattedTime());

      secureEspClient.setX509Time(timeClient.getEpochTime());
      secureEspClient.setInsecure(); // does not check certificate chain. TODO: implement check of fingerprint.
      client.setClient(secureEspClient);
      Serial.println("(secure)");
    }
    else
    {
      client.setClient(espClient);
      Serial.println("(NOT secure)");
    }

    client.setServer(Settings.mqtt_host, Settings.mqtt_port);

    // Attempt to connect
    if (client.connect(Settings.mqtt_client, Settings.mqtt_user, Settings.mqtt_pwd))
    {
      Serial.println("connected");

      subscribeToMqtt();
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.println(client.state());

      char buf[256];
      secureEspClient.getLastSSLError(buf, 256);
      Serial.print("WiFiClientSecure SSL error: ");
      Serial.println(buf);
    }

    lastReconnect = millis();
  }
}

void subscribeToMqtt()
{
  // subscribe all actors to mqtt
  for (byte i = 0; i < (sizeof(Settings.actors) / sizeof(*Settings.actors)); i++)
  {
    if (Settings.actors[i].topic[0] != '\0')
    {
      if (client.subscribe(Settings.actors[i].topic))
      {
        Serial.println("Successfully subscribed");
      }
      else
      {
        Serial.println("Failed to subscribe....");
      }
    }
  }
}

void loop()
{
  handleWebserver();
  timeClient.update();

  // set pin of actor to low when ticks_until passed.
  for (byte i = 0; i < (sizeof(Settings.actors) / sizeof(*Settings.actors)); i++)
  {
    if (Settings.actors[i].ticks_until && Settings.actors[i].ticks_until < millis() && digitalRead(Settings.actors[i].pin))
    {
      Serial.print("Setting actor to low: ");
      Serial.println(Settings.actors[i].topic);

      digitalWrite(Settings.actors[i].pin, LOW);
      Settings.actors[i].ticks_until = 0;
    }
  }

  if (!client.connected())
  {
    reconnect();
  }

  if (client.connected())
  {
    client.loop();

    // send each 60s sensor data to cloud.
    long now = millis();
    if (now - lastSensorDataSent > 60000)
    {
      lastSensorDataSent = now;

      sendSensorData();
    }
  }
}

void sendSensorData()
{
  for (byte i = 0; i < (sizeof(Settings.sensors) / sizeof(*Settings.sensors)); i++)
  {
    if (Settings.sensors[i].sensorName[0] != '\0' && Settings.sensors[i].location[0] != '\0')
    {
      String payload;

      // read sensor data
      int val = analogRead(Settings.sensors[i].pin);

      /* 
        Create json payload.
        
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
      JsonObject &root = jsonBuffer.createObject();
      root["sensorName"] = Settings.sensors[i].sensorName;
      root["type"] = Settings.sensors[i].type;
      root["value"] = val;
      root["location"] = Settings.sensors[i].location;
      root["version"] = 1;

      root.printTo(payload);

      Serial.print("Sending: ");
      Serial.println(payload);
      char payloadChar[payload.length() + 1];

      payload.toCharArray(payloadChar, payload.length() + 1);

      client.publish("habarama", payloadChar, true);
    }
  }
}
