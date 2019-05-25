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

// configuration for sensor
#define sensor_name "......"
#define sensor_type "......"
#define sensor_location "......"

/* ntp */
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

/* mqtt */
WiFiClientSecure secureEspClient;
WiFiClient espClient;
PubSubClient client;

/* Webserver */
ESP8266WebServer http_rest_server(80);

// configuration for actor.
actor_type actors[] = {
    // {D0, 0, "actor.<location>.<actor name 1>"},
    // {D1, 0, "actor.<location>.<actor name 2>"}
};

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
}

void initializeWebServer()
{
  http_rest_server.on("api/config/mqtt", HTTP_GET, http_get_mqtt_config);
  http_rest_server.on("api/config/mqtt", HTTP_PUT, http_put_mqtt_config);

  http_rest_server.begin();
}

void initializeWifi()
{
  WiFiManager wifiManager;
  // wifiManager.resetSettings(); // This can be used to rest wifi settings.
  wifiManager.autoConnect();
}

void initializeActors()
{
  for (byte i = 0; i < (sizeof(actors) / sizeof(*actors)); i++)
  {
    actors[i].ticks_until = 0;
    pinMode(actors[i].pin, OUTPUT);
    digitalWrite(actors[i].pin, LOW);
  }
}

void initializeMqtt()
{
  client.setCallback(mqtt_callback);
}

void http_get_mqtt_config()
{
  // curl -i -X GET http://192.168.43.161/api/config/mqtt

  Serial.println("Start get mqtt config");
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject &jsonObj = jsonBuffer.createObject();
  char JSONmessageBuffer[200];

  /*
    uint8_t       mqtt_fingerprint[2][20];
    char          mqtt_host[33];
    uint16_t      mqtt_port;
    char          mqtt_client[33];
    char          mqtt_user[33];
    char          mqtt_pwd[33];
    bool          mqtt_ssl;
  */

  //jsonObj["fingerprint"] = Settings.mqtt_fingerprint;
  jsonObj["host"] = Settings.mqtt_host;
  jsonObj["port"] = Settings.mqtt_port;
  jsonObj["client"] = Settings.mqtt_client;
  jsonObj["user"] = Settings.mqtt_user;
  jsonObj["pwd"] = Settings.mqtt_pwd;
  jsonObj["ssl"] = Settings.mqtt_ssl;
  jsonObj.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));

  Serial.print(http_rest_server.method());
  Serial.print(" - ");
  Serial.print(http_rest_server.uri());
  Serial.print(" - ");
  Serial.print(JSONmessageBuffer);

  http_rest_server.send(200, "application/json", JSONmessageBuffer);

  Serial.println("End get mqtt config");
}

void http_put_mqtt_config()
{
  // curl -i -X PUT -d {\"host\":\"host\",\"port\":1234,\"client\":\"asdf\",\"user\":\"user\",\"pwd\":\"password\",\"ssl\":false} http://192.168.43.161/api/config/mqtt
  
  StaticJsonBuffer<500> jsonBuffer;
  String post_body = http_rest_server.arg("plain");
  Serial.println(post_body);

  JsonObject &jsonBody = jsonBuffer.parseObject(post_body);

  if (jsonBody.success())
  {

    /*
      uint8_t       mqtt_fingerprint[2][20];
      char          mqtt_host[33];
      uint16_t      mqtt_port;
      char          mqtt_client[33];
      char          mqtt_user[33];
      char          mqtt_pwd[33];
    */

    //  Settings.mqtt_fingerprint = jsonBody["fingerprint"];
    strlcpy(Settings.mqtt_host, jsonBody["host"], sizeof(Settings.mqtt_host));
    Settings.mqtt_port = jsonBody["port"];
    strlcpy(Settings.mqtt_client, jsonBody["client"], sizeof(Settings.mqtt_client));
    strlcpy(Settings.mqtt_user, jsonBody["user"], sizeof(Settings.mqtt_user));
    strlcpy(Settings.mqtt_pwd, jsonBody["pwd"], sizeof(Settings.mqtt_pwd));
    Settings.mqtt_ssl = jsonBody["ssl"];

    saveSettings();
    http_rest_server.send(200);
  }
  else
  {
    http_rest_server.send(400);
  }
}

void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("mqtt topic: ");
  Serial.println(topic);

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject &root = jsonBuffer.parseObject(payload);

  for (byte i = 0; i < (sizeof(actors) / sizeof(*actors)); i++)
  {
    // Check if topic matches to actor
    if (strcmp(topic, actors[i].topic.c_str()) == 0)
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
        setGpio(&actors[i], duration);
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
  if (!client.connected() && (lastReconnect == 0 || (millis() - lastReconnect) > 10000))
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
      secureEspClient.setInsecure();
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
    if (client.connect("ESP8266Client", Settings.mqtt_user, Settings.mqtt_pwd))
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
  for (byte i = 0; i < (sizeof(actors) / sizeof(*actors)); i++)
  {
    if (actors[i].topic.length())
    {
      if (client.subscribe(actors[i].topic.c_str()))
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

  http_rest_server.handleClient();
  timeClient.update();

  // set pin of actor to low when ticks_until passed.
  for (byte i = 0; i < (sizeof(actors) / sizeof(*actors)); i++)
  {
    if (actors[i].ticks_until < millis() && digitalRead(actors[i].pin))
    {
      Serial.println("Setting actor to low: ");
      Serial.println(actors[i].topic);

      digitalWrite(actors[i].pin, LOW);
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
  JsonObject &root = jsonBuffer.createObject();
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
