

## Hardware

### Required components
* ESP8266 ESP-12E ([Amazon](http://www.amazon.de/dp/B06Y1LZLLY/))
* Moisture sensor ([Amazon](http://www.amazon.de/dp/B075K94S5D/))

### Schematics
<img src="Schematics/moisture_sensor.png" height="500px">

## Software

IDE: [Arduino](https://www.arduino.cc/en/Main/Software)

### Dependencies
* ESP8266 Board ([Github](https://github.com/esp8266/Arduino))
* PubSubClient 2.6.0 ([Github](https://github.com/knolleary/pubsubclient))
* ArduinoJson 5.13.2 ([Github](https://github.com/bblanchon/ArduinoJson))

## Installation

1. Install Arduino IDE from link above
1. Add ESP8266 Addon (See tutorial from [Sparkfun](https://learn.sparkfun.com/tutorials/esp8266-thing-hookup-guide/installing-the-esp8266-arduino-addon))
1. Install the PubSubClient and ArduinoJson library
   * *Sketch > Include Library > Manage Libraries*
   * Search for PubSubClient and ArduinoJson and install the appropriate versions.
1. Download this repository and open it with the IDE
1. Enter the SSID and Password of the Wifi where ESP8266 will connect to in the following two defines:
    ```` 
    #define wifi_ssid "......."
    #define wifi_password "........"
    ````
1. Also enter the `name`, `type` and `location` of the sensor:
    ````
    #define sensor_name "......"
    #define sensor_type "......"
    #define sensor_location "......"
    ````
1. Connect ESP8266 to the computer
1. Select the board from *Tools > Board > NodeMCU 1.0 (ESP-12E Module)*
1. Select the COM port where ESP8266 is connected *Tools > Board > Port > COMxx*
1. Push `RST` and `FLASH` button, release `RST` and then release `FLASH`
1. Download the firmware from the Arduino IDE
