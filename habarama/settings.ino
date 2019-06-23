#include "FS.h"


bool loadSettings() {
  
  SPIFFS.begin();
  
  File configFile = SPIFFS.open("/config.dat", "r");
  if (!configFile) {
    Serial.println("Failed to open config file");
    return false;
  }

  size_t size = configFile.size();

  if (size != sizeof(settings_type)) {
    Serial.print("Invalid size: ");
    Serial.println(size);
    return false;
  }

  configFile.readBytes((char*)&Settings, sizeof(Settings));

  Serial.print("mqtt host: ");
  Serial.print(Settings.mqtt_host);
  Serial.print(":");
  Serial.println(Settings.mqtt_port);
  return true;
}

bool saveSettings() {

  SPIFFS.begin();
  
  File configFile = SPIFFS.open("/config.dat", "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }

  configFile.write((const uint8_t*)&Settings, sizeof(settings_type));
  return true;
}
