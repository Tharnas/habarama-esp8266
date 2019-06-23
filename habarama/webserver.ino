
/* Webserver */
ESP8266WebServer http_rest_server(80);

void initializeWebServer()
{
    http_rest_server.on("/api/config/mqtt", HTTP_GET, http_get_mqtt_config);
    http_rest_server.on("/api/config/mqtt", HTTP_PUT, http_put_mqtt_config);

    http_rest_server.on("/api/config/actors", HTTP_GET, http_get_actors_config);
    http_rest_server.on("/api/config/actors", HTTP_PUT, http_put_actors_config);

    http_rest_server.on("/api/config/sensors", HTTP_GET, http_get_sensors_config);
    http_rest_server.on("/api/config/sensors", HTTP_PUT, http_put_sensors_config);

    http_rest_server.onNotFound(handleNotFound);

    http_rest_server.begin();
}

void handleWebserver()
{
    http_rest_server.handleClient();
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
    Serial.println(JSONmessageBuffer);

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

void http_get_actors_config()
{
    // curl -i -X GET http://192.168.43.161/api/config/actors

    Serial.println("Start get actors config");
    StaticJsonBuffer<200> jsonBuffer;
    JsonArray &jsonArray = jsonBuffer.createArray();
    char JSONmessageBuffer[200];

    for (byte i = 0; i < (sizeof(Settings.actors) / sizeof(*Settings.actors)); i++)
    {
        if (Settings.actors[i].topic[0] != '\0')
        {
            JsonObject &jsonObj = jsonBuffer.createObject();

            jsonObj["pin"] = Settings.actors[i].pin;
            jsonObj["topic"] = Settings.actors[i].topic;

            jsonArray.add(jsonObj);
        }
    }

    jsonArray.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));

    Serial.print(http_rest_server.method());
    Serial.print(" - ");
    Serial.print(http_rest_server.uri());
    Serial.print(" - ");
    Serial.println(JSONmessageBuffer);

    http_rest_server.send(200, "application/json", JSONmessageBuffer);

    Serial.println("End get actor config");
}

void http_put_actors_config()
{
    // curl -i -X PUT -d [{\"pin\":1,\"topic\":\"plant.test\"}] http://192.168.43.161/api/config/actors

    StaticJsonBuffer<500> jsonBuffer;
    String post_body = http_rest_server.arg("plain");
    Serial.println(post_body);

    JsonArray &jsonBody = jsonBuffer.parseArray(post_body);

    if (jsonBody.success())
    {
        /*
    [
      {
        byte pin; // the pin where the actor is connected to
        char topic[33]; // the topic this actor should listen to   
      },
      { ... }
    ]
    */

        for (byte i = 0; i < (sizeof(Settings.actors) / sizeof(*Settings.actors)); i++)
        {
            if (i < jsonBody.size())
            {
                Settings.actors[i].pin = jsonBody[i]["pin"].as<byte>();
                strlcpy(Settings.actors[i].topic, jsonBody[i]["topic"], sizeof(Settings.actors[i].topic));
                Settings.actors[i].ticks_until = 0;
            }
            else
            {
                // reset
                Settings.actors[i].pin = 0;
                memset(&Settings.actors[i].topic[0], 0, sizeof(Settings.actors[i].topic));
                Settings.actors[i].ticks_until = 0;
            }
        }

        saveSettings();
        http_rest_server.send(200);
    }
    else
    {
        http_rest_server.send(400);
    }
}

void http_get_sensors_config()
{
    // curl -i -X GET http://192.168.43.161/api/config/sensors

    Serial.println("Start get sensors config");
    StaticJsonBuffer<200> jsonBuffer;
    JsonArray &jsonArray = jsonBuffer.createArray();
    char JSONmessageBuffer[200];

    for (byte i = 0; i < (sizeof(Settings.sensors) / sizeof(*Settings.sensors)); i++)
    {
        Serial.print("sensor: ");
        Serial.println(i);
        Serial.print("sensorName: ");
        Serial.println(Settings.sensors[i].sensorName);
        Serial.print("location: ");
        Serial.println(Settings.sensors[i].location);

        if (Settings.sensors[i].sensorName[0] != '\0' && Settings.sensors[i].location[0] != '\0')
        {
            JsonObject &jsonObj = jsonBuffer.createObject();

            jsonObj["pin"] = Settings.sensors[i].pin;
            jsonObj["sensorName"] = Settings.sensors[i].sensorName;
            jsonObj["location"] = Settings.sensors[i].location;
            jsonObj["type"] = Settings.sensors[i].type;

            jsonArray.add(jsonObj);
        }
    }

    jsonArray.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));

    Serial.print(http_rest_server.method());
    Serial.print(" - ");
    Serial.print(http_rest_server.uri());
    Serial.print(" - ");
    Serial.println(JSONmessageBuffer);

    http_rest_server.send(200, "application/json", JSONmessageBuffer);

    Serial.println("End get sensor config");
}

void http_put_sensors_config()
{
    // curl -i -X PUT -d "[{\"pin\":0,\"sensorName\":\"MyPlant\",\"location\":\"Planet 1\",\"type\":\"water\"}]" http://192.168.43.161/api/config/sensors

    StaticJsonBuffer<500> jsonBuffer;
    String post_body = http_rest_server.arg("plain");
    Serial.println(post_body);

    JsonArray &jsonBody = jsonBuffer.parseArray(post_body);

    if (jsonBody.success())
    {
        /*
        [
            {
                byte pin;            // the pin where the sensor is connected to
                char sensorName[33]; // name of the sensor that will be sent to cloud
                char location[33];   // location of the sensor that will be sent to cloud
                char type[33];       // water, ligth, ...
            }
            { ... }
        ]
        */

        for (byte i = 0; i < (sizeof(Settings.sensors) / sizeof(*Settings.sensors)); i++)
        {
            if (i < jsonBody.size())
            {
                Settings.sensors[i].pin = jsonBody[i]["pin"].as<byte>();
                strlcpy(Settings.sensors[i].sensorName, jsonBody[i]["sensorName"], sizeof(Settings.sensors[i].sensorName));
                strlcpy(Settings.sensors[i].location, jsonBody[i]["location"], sizeof(Settings.sensors[i].location));
                strlcpy(Settings.sensors[i].type, jsonBody[i]["type"], sizeof(Settings.sensors[i].type));
            }
            else
            {
                // reset
                Settings.sensors[i].pin = 0;
                memset(&Settings.sensors[i].sensorName[0], 0, sizeof(Settings.sensors[i].sensorName));
                memset(&Settings.sensors[i].location[0], 0, sizeof(Settings.sensors[i].location));
                memset(&Settings.sensors[i].type[0], 0, sizeof(Settings.sensors[i].type));
            }

            Serial.print("sensor: ");
            Serial.println(i);
            Serial.print("sensorName: ");
            Serial.println(Settings.sensors[i].sensorName);
            Serial.print("location: ");
            Serial.println(Settings.sensors[i].location);
        }

        saveSettings();
        http_rest_server.send(200);
    }
    else
    {
        http_rest_server.send(400);
    }
}

void handleNotFound()
{
    Serial.print("Unhandled request: ");
    Serial.print(http_rest_server.method());
    Serial.print(" - ");
    Serial.println(http_rest_server.uri());

    http_rest_server.send(404, "text/plain", "404: Whaaaaaat?");
}
