
#ifndef _SETTINGS_H_
#define _SETTINGS_H_

#define MAX_ACTORS 5
#define MAX_SENSORS 1

struct actor_type
{
  byte pin;         // the pin where the actor is connected to
  long ticks_until; // the ticks until the pin should be triggered. Should be initialized with 0.
  char topic[33];   // the topic this actor should listen to
};

struct sensor_type
{
  byte pin;            // the pin where the sensor is connected to
  char sensorName[33]; // name of the sensor that will be sent to cloud
  char location[33];   // location of the sensor that will be sent to cloud
  char type[33];       // water, ligth, ...
};

struct settings_type
{
  uint16_t cfg_size;
  uint16_t cfg_crc;
  uint8_t mqtt_fingerprint[2][20];
  char mqtt_host[33];
  uint16_t mqtt_port;
  char mqtt_client[33];
  char mqtt_user[33];
  char mqtt_pwd[33];
  bool mqtt_ssl;

  actor_type actors[MAX_ACTORS];
  sensor_type sensors[MAX_SENSORS];
} Settings;

#endif // _SETTINGS_H_
