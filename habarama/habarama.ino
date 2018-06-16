#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#define wifi_ssid "......."
#define wifi_password "........"

#define sensor_name "......"
#define sensor_type "......"
#define sensor_location "......"


#define mqtt_server "broker-amq-mqtt-ssl-57-hogarama.cloud.itandtel.at"
#define mqtt_server_port 443
#define mqtt_user "mq_habarama"
#define mqtt_password "mq_habarama_pass"

WiFiClientSecure espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  setup_wifi();
  espClient.allowSelfSignedCerts();
  client.setServer(mqtt_server, mqtt_server_port);
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    // Attempt to connect
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      Serial.println("connected");
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

  long now = millis();
  if (now - lastMsg > 60000) {
    lastMsg = now;

    int val = val = analogRead(0);

    String payload;
    StaticJsonBuffer<200> jsonBuffer;

    JsonObject& root = jsonBuffer.createObject();
    //  '{{"sensorName": "name", "type": "type", "value": value, "location": "location", "version": 1 }}'
    root["sensorName"] = sensor_name;
    root["type"] = sensor_type;
    root["value"] = map(val, 150, 430, 100, 0); 
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


