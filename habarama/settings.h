
#ifndef _SETTINGS_H_
#define _SETTINGS_H_


#define MAX_ACTORS 5
struct actor_type {
  byte pin; // the pin where the actor is connected to
  long ticks_until; // the ticks until the pin should be triggered. Should be initialized with 0.
  String topic; // the topic this actor should listen to
};

struct settings_type {
  uint16_t      cfg_size;
  uint16_t      cfg_crc;
  uint8_t       mqtt_fingerprint[2][20];
  char          mqtt_host[33];       
  uint16_t      mqtt_port;           
  char          mqtt_client[33];     
  char          mqtt_user[33];
  char          mqtt_pwd[33];

  actor_type    actors[MAX_ACTORS];
} Settings;


#endif // _SETTINGS_H_
